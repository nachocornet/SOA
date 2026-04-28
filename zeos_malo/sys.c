/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

extern page_table_entry *PT_systemAddress;
extern void ret_from_fork(void);

#define LECTURA 0
#define ESCRIPTURA 1


int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
    return -38; /*ENOSYS*/
}

/* ret_from_fork is implemented in entry.S and should return to sysenter_fin */

#include <io.h>
#include <system.h> 

static char sys_buffer[256]; 
int next_PID = 2;

int sys_write(int fd, char *buffer, int size) {
    int error;
    int bytes_left = size;
    int bytes_written = 0;

    error = check_fd(fd, ESCRIPTURA); 
    if (error < 0) return error; 

    if (size < 0) return -22;    
    if (size == 0) return 0;
    
    if (buffer == NULL) return -14; 
    error = access_ok(ESCRIPTURA, buffer, size);
    if (!error) return -14;

    while (bytes_left > 0) {
        int chunk_size = (bytes_left > 256) ? 256 : bytes_left;
        
        copy_from_user(buffer + bytes_written, sys_buffer, chunk_size);
        
        sys_write_console(sys_buffer, chunk_size);
        
        bytes_written += chunk_size;
        bytes_left -= chunk_size;
    }
    
    return bytes_written; 
}

int sys_read(char *buffer, int maxchars)
{
    int bytes_read = 0;
    char c;

    if (maxchars < 0) return -22; /* EINVAL */
    if (maxchars == 0) return 0;

    if (buffer == NULL) return -14; /* EFAULT */
    if (!access_ok(LECTURA, buffer, maxchars)) return -14;

    while (bytes_read < maxchars) {
        if (keyboard_buffer_pop(&c) == 0) {
            copy_to_user(&c, buffer + bytes_read, 1);
            bytes_read++;
            continue;
        }

        /* Blocking behavior: sleep until a new key arrives. */
        keyboard_block_current_reader();
    }

    return bytes_read;
}

extern int zeos_ticks;
extern unsigned long get_ebp(void);

int sys_gettime()
{
    return zeos_ticks;
}

int sys_getpid()
{
    return current()->PID;
}

int fork_nomem(int *frames, struct task_struct *child) {
    for (int i = 0; i < NUM_PAG_DATA; ++i)
        if (frames[i] != -1) free_frame(frames[i]);

    if (child->dir_pages_baseAddr != NULL) {
        unsigned int dir_frame = ((unsigned int)child->dir_pages_baseAddr) >> 12;
        unsigned int pt_user_frame = child->dir_pages_baseAddr[1].bits.pbase_addr;
        free_frame(pt_user_frame);
        free_frame(dir_frame);
        child->dir_pages_baseAddr = NULL;
    }

    free_task_struct(child);
    return -12;
}

int sys_fork()
{
    int i;
    int frames[NUM_PAG_DATA];
    struct task_struct *parent = current();

    for (i = 0; i < NUM_PAG_DATA; ++i) frames[i] = -1;

    struct task_struct *child = alloc_task_struct();
    if (child == NULL) return -12; /* ENOMEM */

    union task_union *parent_u = (union task_union *)parent;
    union task_union *child_u  = (union task_union *)child;

    copy_data(parent_u, child_u, sizeof(union task_union));
    INIT_LIST_HEAD(&child->list);

    child->dir_pages_baseAddr = NULL;
    allocate_DIR(child);
    if (child->dir_pages_baseAddr == NULL) {
        free_task_struct(child);
        return -12;
    }

    page_table_entry *PT_parent = get_PT(parent);
    page_table_entry *PT_child  = get_PT(child);

    /* Share user code pages (read-only). */
    for (i = 0; i < NUM_PAG_CODE; ++i)
        PT_child[NUM_PAG_DATA + i].entry = PT_parent[NUM_PAG_DATA + i].entry;

    for (i = 0; i < NUM_PAG_DATA; ++i) {
        frames[i] = alloc_frame();
        if (frames[i] == -1) return fork_nomem(frames, child);
        set_ss_pag(PT_child, i, frames[i], 1);
    }

    /* Copy user data+stack into child frames using a temporary parent mapping. */
    {
        const unsigned temp_page = NUM_PAG_DATA + NUM_PAG_CODE;
        for (i = 0; i < NUM_PAG_DATA; ++i) {
            set_ss_pag(PT_parent, temp_page, frames[i], 1);
            set_cr3(parent->dir_pages_baseAddr);

            copy_data((void *)((PAG_LOG_INIT_DATA + i) << 12),
                      (void *)((PAG_LOG_INIT_DATA + temp_page) << 12),
                      PAGE_SIZE);

            del_ss_pag(PT_parent, temp_page);
        }
        set_cr3(parent->dir_pages_baseAddr);
    }

    child->PID = next_PID++;
    child->quantum = parent->quantum;
    INIT_LIST_HEAD(&child->children);
    INIT_LIST_HEAD(&child->sibling);
    child->parent = parent;
    child->pending_unblocks = 0;
    list_add_tail(&child->sibling, &parent->children);

    /*
     * Build the first child kernel context explicitly:
     * - switch_stack will pop EBP and RET into ret_from_fork
     * - ret_from_fork jumps to sysenter_fin, which expects the saved
     *   syscall frame (SAVE_ALL + user return context) on the stack.
     */
    unsigned long parent_ebp = get_ebp();
    unsigned long offset = parent_ebp - (unsigned long)parent_u;
    unsigned long *child_ebp = (unsigned long *)((unsigned long)child_u + offset);

    child_ebp[-1] = 0;
    child_ebp[0] = (unsigned long)ret_from_fork;
    --child_ebp;
    child->kernel_esp = (int)child_ebp;

    update_process_state_rr(child, &readyqueue);
    return child->PID;

}


void sys_exit(void) {
    struct task_struct *p = current();
    struct list_head *pos, *n;
    page_table_entry *PT = get_PT(p);

    reap_terminated_tasks();

    if (p->parent != NULL) {
        list_del(&p->sibling);
        p->parent = NULL;
    }

    /* Reparent alive children to idle task. */
    list_for_each_safe(pos, n, &p->children) {
        struct task_struct *child = list_entry(pos, struct task_struct, sibling);
        list_del(&child->sibling);
        list_add_tail(&child->sibling, &idle_task->children);
        child->parent = idle_task;
    }

    // Free user data pages (they are at indices 0 to NUM_PAG_DATA - 1 in the user PT)
    for (int i = 0; i < NUM_PAG_DATA; ++i) {
        unsigned int frame = get_frame(PT, i);
        if (frame != 0) {
            free_frame(frame);
            del_ss_pag(PT, i);
        }
    }

    // Free page table and directory
    if (p->dir_pages_baseAddr != NULL) {
        unsigned int dir_frame = ((unsigned int)p->dir_pages_baseAddr) >> 12;
        unsigned int pt_user_frame = p->dir_pages_baseAddr[1].bits.pbase_addr;
        free_frame(pt_user_frame);
        free_frame(dir_frame);
        p->dir_pages_baseAddr = NULL;
    }

    // Defer releasing the PCB page until we are running on another kernel stack
    INIT_LIST_HEAD(&p->children);
    INIT_LIST_HEAD(&p->sibling);
    p->PID = -1;
    p->pending_unblocks = 0;
    p->state = ST_FREE;
    defer_free_current_task(p);

    if (!list_empty(&readyqueue)) {
        sched_next_rr();
    } else {
        task_switch((union task_union *)idle_task);
    }

    while (1);
}

void sys_block(void)
{
    struct task_struct *p = current();

    if (p->pending_unblocks > 0) {
        p->pending_unblocks--;
        return;
    }

    update_process_state_rr(p, &blocked);

    if (!list_empty(&readyqueue)) {
        sched_next_rr();
    } else {
        task_switch((union task_union *)idle_task);
    }
}

int sys_unblock(int pid)
{
    struct task_struct *parent = current();
    struct task_struct *child = NULL;
    struct list_head *pos;
    
    list_for_each(pos, &parent->children) {
        struct task_struct *t = list_entry(pos, struct task_struct, sibling);
        if (t->PID == pid) {
            child = t;
            break;
        }
    }

    if (child == NULL) return -1;

    if (child->state == ST_BLOCKED) {
        update_process_state_rr(child, &readyqueue);
    } else {
        child->pending_unblocks++;
    }
    
    return 0;
}
