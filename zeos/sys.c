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
    if (error < 0) return error;

    while (bytes_left > 0) {
        int chunk_size = (bytes_left > 256) ? 256 : bytes_left;
        
        copy_from_user(buffer + bytes_written, sys_buffer, chunk_size);
        
        sys_write_console(sys_buffer, chunk_size);
        
        bytes_written += chunk_size;
        bytes_left -= chunk_size;
    }

    return bytes_written; 
}

extern int zeos_ticks;
extern unsigned long get_esp(void);
extern unsigned long get_ebp(void);

int sys_gettime()
{
    return zeos_ticks;
}

int sys_getpid()
{
    return current()->PID;
}

int sys_fork()
{
    int i;
    int frames[NUM_PAG_DATA];

    // 2) 
    if (list_empty(&freequeue)) {
        return -12; /* ENOMEM */
    }

    struct list_head *free_item = list_first(&freequeue);
    list_del(free_item);
    struct task_struct *child = list_head_to_task_struct(free_item);

    // 3) 
    union task_union *parent_u = (union task_union *) current();
    union task_union *child_u  = (union task_union *) child;

    copy_data(parent_u, child_u, sizeof(union task_union));
    INIT_LIST_HEAD(&child->list);

    /* Directorios + tablas nuevas del hijo */
    child->dir_pages_baseAddr = NULL;
    allocate_DIR(child);
    if (child->dir_pages_baseAddr == NULL) {
        list_add_tail(&child->list, &freequeue);
        return -12; /* ENOMEM */
    }

    struct task_struct *parent = current();
    unsigned int PT_parent_frame = parent->dir_pages_baseAddr[1].bits.pbase_addr;
    page_table_entry *PT_parent = (page_table_entry *)(PT_parent_frame << 12);

    unsigned int PT_child_frame = child->dir_pages_baseAddr[1].bits.pbase_addr;
    page_table_entry *PT_child = (page_table_entry *)(PT_child_frame << 12);

    for (i = 0; i < NUM_PAG_DATA; i++) {
        frames[i] = alloc_frame();
        if (frames[i] == -1) {
            for (int j = 0; j < i; j++) {
                free_frame(frames[j]);
            }
            list_add_tail(&child->list, &freequeue);
            return -12;
        }
    }

    for (i = 0; i < NUM_PAG_CODE; i++) {
        PT_child[PAG_LOG_INIT_CODE + i].entry = PT_parent[PAG_LOG_INIT_CODE + i].entry;
    }

    const unsigned temp_page = PAG_LOG_INIT_DATA + NUM_PAG_DATA + NUM_PAG_CODE;

    for (i = 0; i < NUM_PAG_DATA; i++) {
        set_ss_pag(PT_child, PAG_LOG_INIT_DATA + i, frames[i], 1);

        set_ss_pag(PT_parent, temp_page, frames[i], 1);
        set_cr3(current()->dir_pages_baseAddr);

        void *src = (void *) ((PAG_LOG_INIT_DATA + i) << 12);
        void *dst = (void *) (temp_page << 12);
        copy_data(src, dst, PAGE_SIZE);

        del_ss_pag(PT_parent, temp_page);
    }

    set_cr3(current()->dir_pages_baseAddr);

    child->PID = next_PID++;
    child->quantum = current()->quantum;

    unsigned long parent_ebp = get_ebp();
    unsigned long parent_stack_base = (unsigned long)&parent_u->stack[0];
    unsigned long child_stack_base = (unsigned long)&child_u->stack[0];
    unsigned long offset_ebp = parent_ebp - parent_stack_base;
    child->kernel_esp = (int)(child_stack_base + offset_ebp);

    unsigned long *child_ebp = (unsigned long *)(child_stack_base + offset_ebp);
    child_ebp[0] = 0;
    child_ebp[1] = (unsigned long) ret_from_fork;

    /* Add the new child to the ready queue for the scheduler. */
    update_process_state_rr(child, &readyqueue);

    return child->PID;
}

void sys_exit(void) {
    /* Exit is not implemented yet. The process destruction logic will be added later. */
    while (1);
}
