/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

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
extern void ret_from_fork(void);

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

    if (list_empty(&freequeue))
        return -12; /* ENOMEM */

    struct list_head *free_item = list_first(&freequeue);
    list_del(free_item);
    struct task_struct *child = list_head_to_task_struct(free_item);

    union task_union *parent_u = (union task_union *) current();
    union task_union *child_u  = (union task_union *) child;

    /* Copiamos la unión completa, incluida la pila de kernel */
    copy_data(parent_u, child_u, sizeof(union task_union));

    /* El list_head del hijo debe reiniciarse para evitar corrupción de colas */
    INIT_LIST_HEAD(&child->list);

    /* Directorios + tablas nuevas del hijo */
    allocate_DIR(child);

    //page_table_entry *PT_parent = get_PT(current());
    page_table_entry *PT_parent = (page_table_entry *)(((unsigned int)(current()->dir_pages_baseAddr[1].bits.pbase_addr))<<12);
    //page_table_entry *PT_child  = get_PT(child);
    page_table_entry *PT_child  = (page_table_entry *)(((unsigned int)(child->dir_pages_baseAddr[1].bits.pbase_addr))<<12);

    /* 5) Asignar frames físicos para las páginas de datos del hijo */
    for (i = 0; i < NUM_PAG_DATA; i++) {
        frames[i] = alloc_frame();
        if (frames[i] == -1) return -12;
    }

    /* 6) Compartir código (páginas de código de usuario) */
    for (i = 0; i < NUM_PAG_CODE; i++) {
        PT_child[PAG_LOG_INIT_CODE + i].entry = PT_parent[PAG_LOG_INIT_CODE + i].entry;
    }

    /* 7) Copiar páginas de datos una por una con mapeado temporal en el padre */
    const unsigned temp_page = PAG_LOG_INIT_DATA + NUM_PAG_DATA + NUM_PAG_CODE;

    for (i = 0; i < NUM_PAG_DATA; i++) {
        set_ss_pag(PT_child, PAG_LOG_INIT_DATA + i, frames[i], 1);

        /* Mapeo temporal en el padre para escribir en la física recién asignada */
        set_ss_pag(PT_parent, temp_page, frames[i], 1);
        set_cr3(get_DIR(current())); /* flush TLB */

        void *src = (void *) ((PAG_LOG_INIT_DATA + i) << 12);
        void *dst = (void *) (temp_page << 12);
        copy_data(src, dst, PAGE_SIZE);

        del_ss_pag(PT_parent, temp_page);
        set_cr3(get_DIR(current())); /* flush TLB tras desenlazar */
    }

    /* 8) flush final del TLB tras montar estructuras nuevas */
    set_cr3(get_DIR(current()));

    /* 9) PID hijo */
    child->PID = next_PID++;

    /* 10) Preparar pila de sistema del hijo para salir con ret_from_fork */
    child_u->stack[KERNEL_STACK_SIZE - 18] = (unsigned long) ret_from_fork;
    child_u->stack[KERNEL_STACK_SIZE - 19] = 0; /* EBP falso */
    child->kernel_esp = (int) &child_u->stack[KERNEL_STACK_SIZE - 19];

    /* 11) Encolar el hijo en readyqueue */
    list_add_tail(&child->list, &readyqueue);

    return child->PID;
}

