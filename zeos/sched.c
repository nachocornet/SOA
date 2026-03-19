/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <hardware.h>

char initial_stack[KERNEL_STACK_SIZE]; // Space for the initial system stack

union task_union task[NR_TASKS];
struct list_head freequeue;
struct list_head readyqueue;
struct task_struct *init_task;
struct task_struct *idle_task;

int PT_system;
page_table_entry *PT_systemAddress;

extern void writeMSR(unsigned int msr_number, unsigned int value);

void cpu_idle(void)
{
	__sti();
	while(1)
	{
	;
	}
}

void init_idle (void)
{
	// 1) 
	int Dir = alloc_frame();
	page_table_entry *DirAddress = (page_table_entry *) (Dir << 12);
	clear_page_table(DirAddress);

	DirAddress[0].entry = 0;
	DirAddress[0].bits.pbase_addr = PT_system;
	DirAddress[0].bits.rw = 1;
	DirAddress[0].bits.present = 1;
	set_ss_pag(PT_systemAddress, Dir, Dir, 0);

	// 4)
	struct list_head *first = list_first(&freequeue);
	list_del(first);
	struct task_struct *t = list_head_to_task_struct(first);
	
	// 2)
	t->dir_pages_baseAddr = DirAddress;

	// 3)
	union task_union *tu = (union task_union *)t;
	tu->stack[KERNEL_STACK_SIZE - 1] = (unsigned long)cpu_idle;
	tu->stack[KERNEL_STACK_SIZE - 2] = 0;
	t->kernel_esp = (int)&(tu->stack[KERNEL_STACK_SIZE - 2]);

	// 5)
	int PCB_frame = ((unsigned int)t) >> 12;
	set_ss_pag(PT_systemAddress, PCB_frame, PCB_frame, 0);
	
	// 6)
	t->PID = 0;

	// 7)
	idle_task = t;
	

}

void init_task1(void)
{
	// 1)
	// a)
	int Dir = alloc_frame();
	page_table_entry *DirAddress = (page_table_entry *) (Dir << 12);

	// b)
	clear_page_table(DirAddress);

	// c)
	PT_system = alloc_frame();
	PT_systemAddress = (page_table_entry *) (PT_system << 12);
	clear_page_table(PT_systemAddress);

	// d)
	set_kernel_pages(PT_systemAddress);

	// e)
	int PT_user = alloc_frame();
	page_table_entry *PT_userAddress = (page_table_entry *) (PT_user << 12);
	clear_page_table(PT_userAddress);

	// f)
	set_user_pages(PT_userAddress);

	// g)
	set_ss_pag(PT_systemAddress, Dir, Dir, 0);
	set_ss_pag(PT_systemAddress, PT_system, PT_system, 0);
	set_ss_pag(PT_systemAddress, PT_user, PT_user, 0);

	// h)
	DirAddress[0].entry = 0;
	DirAddress[0].bits.pbase_addr = PT_system;
	DirAddress[0].bits.rw = 1;
	DirAddress[0].bits.present = 1;

	// i)
	DirAddress[1].entry = 0;
	DirAddress[1].bits.pbase_addr = PT_user;
	DirAddress[1].bits.rw = 1;
	DirAddress[1].bits.present = 1;
	DirAddress[1].bits.user = 1;

	// 2)
	struct list_head *first = list_first(&freequeue);
	list_del(first);
	struct task_struct *t = list_head_to_task_struct(first);

	// 3)
	int PCB_frame = ((unsigned int)t) >> 12;
	set_ss_pag(PT_systemAddress, PCB_frame, PCB_frame, 0);

	// 4) 
	t->PID = 1;

	// 5)
	union task_union *tu = (union task_union *)t;
	tss.esp0 = (DWord)&(tu->stack[KERNEL_STACK_SIZE]);
	writeMSR(0x175, (DWord)&(tu->stack[KERNEL_STACK_SIZE]));

	// 6)
	t->dir_pages_baseAddr = DirAddress;

	// 7)
	set_cr3(t->dir_pages_baseAddr);

	// 8)
	init_task = t;
}


void init_sched()
{
/* 1. Inicialitzem les cues */
    INIT_LIST_HEAD(&freequeue);
    INIT_LIST_HEAD(&readyqueue);
    
    /* 2. Afegim tots els PCBs (task_union) disponibles a la freequeue */
    for (int i = 0; i < NR_TASKS; i++) {
        task[i].task.PID = -1; // Opcional: marquem com a no usat
        list_add_tail(&(task[i].task.list), &freequeue);
    }
}

void task_switch(union task_union *new) {
	// 1)
	__asm__ __volatile__(
		"pushl %esi\n\t"
		"pushl %edi\n\t"
		"pushl %ebx\n\t"
	);

	// 2)
	inner_task_switch(new);

	// 3)
	__asm__ __volatile__(
		"popl %ebx\n\t"
		"popl %edi\n\t"
		"popl %esi\n\t"
	);
}

void inner_task_switch(union task_union *new) {
	// 1)
	tss.esp0 = (DWord) &(new->stack[KERNEL_STACK_SIZE]); // KERNEL_STACK_SIZE --> new->kernel_esp
	writeMSR(0x175, (DWord) &(new->stack[KERNEL_STACK_SIZE]));

	// 2)
	set_cr3(new->task.dir_pages_baseAddr);

	unsigned int *old_esp = (unsigned int*) &(current()->kernel_esp);
	unsigned int new_esp = new->task.kernel_esp;

	
	__asm__ __volatile__(
		"movl %%ebp, %0\n\t"

		"movl %1, %%esp\n\t"

		"popl %%ebp\n\t"

		"ret\n\t"
		: 
		: "r" (*old_esp), "r" (new_esp)
	);

}


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t)
{
       return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
       return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}

struct task_struct * list_head_to_task_struct(struct list_head *l) {
	return list_entry(l, struct task_struct, list);
}
