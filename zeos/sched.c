/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <hardware.h>

char initial_stack[KERNEL_STACK_SIZE]; // Space for the initial system stack

union task_union task[NR_TASKS] __attribute__((__aligned__(4096)));
struct list_head freequeue;
struct list_head readyqueue;
struct task_struct *init_task;
struct task_struct *idle_task;

int PT_system;
page_table_entry *PT_systemAddress;

static int remaining_ticks;

extern void writeMSR(unsigned int msr_number, unsigned int value);
extern void task_switch(union task_union *new);
extern void switch_stack(int *save_ebp, int *new_esp);

void allocate_DIR(struct task_struct *t) {
	int Dir = alloc_frame();
	if (Dir == -1)
		return; /* Sin memoria, el llamante debe manejarlo */

	int PT_user = alloc_frame();
	if (PT_user == -1) {
		free_frame(Dir);
		return;
	}

	/* PRIMERO: Mapear los frames en PT_systemAddress ANTES de acceder a ellos */
	set_ss_pag(PT_systemAddress, Dir, Dir, 0);
	set_ss_pag(PT_systemAddress, PT_user, PT_user, 0);

	/* Mapear el PCB en PT_systemAddress para que sea accesible con cualquier CR3 */
	unsigned int PCB_frame = ((unsigned int)t) >> 12;
	set_ss_pag(PT_systemAddress, PCB_frame, PCB_frame, 0);

	/* FLUSH TLB para asegurar que los nuevos mapeos sean visibles */
	set_cr3(current()->dir_pages_baseAddr);

	/* AHORA podemos acceder a las direcciones virtuales mapeadas */
	page_table_entry *DirAddress = (page_table_entry *) (Dir << 12);
	page_table_entry *PT_userAddress = (page_table_entry *) (PT_user << 12);

	clear_page_table(DirAddress);
	clear_page_table(PT_userAddress);

	/* Entrada 0: espacio kernel compartido */
	DirAddress[0].entry = 0;
	DirAddress[0].bits.pbase_addr = PT_system;
	DirAddress[0].bits.rw = 1;
	DirAddress[0].bits.present = 1;

	/* Entrada 1: espacio de usuario del proceso */
	DirAddress[1].entry = 0;
	DirAddress[1].bits.pbase_addr = PT_user;
	DirAddress[1].bits.rw = 1;
	DirAddress[1].bits.present = 1;
	DirAddress[1].bits.user = 1;

	/* Guardar directorio en task_struct */
	t->dir_pages_baseAddr = DirAddress;
}


void cpu_idle(void)
{
	__sti();
	printk("Idle task running...\n");
	while(1)
	{
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
	idle_task = t;

	// 5)
	int PCB_frame = ((unsigned int)t) >> 12;
	set_ss_pag(PT_systemAddress, PCB_frame, PCB_frame, 0);
	
	// 6)
	t->PID = 0;
	t->quantum = 1000;
}

void init_task1(void)
{
	// PASO 1: Allocate structures to store process address space

	// 1a) Allocate a new directory
	int Dir = alloc_frame();
	page_table_entry *DirAddress = (page_table_entry *) (Dir << 12);

	// 1b) Initialize all directory entries
	clear_page_table(DirAddress);

	// 1c) Allocate a page table to store system mappings (ONLY ONCE, globally)
	// 1d) Initialize used system pages on this system page table
	if (PT_system == 0) {
		PT_system = alloc_frame();
		PT_systemAddress = (page_table_entry *) (PT_system << 12);
		clear_page_table(PT_systemAddress);
		set_kernel_pages(PT_systemAddress);
	}

	// 1e) Allocate a page table to store user mappings
	int PT_user = alloc_frame();
	page_table_entry *PT_userAddress = (page_table_entry *) (PT_user << 12);
	clear_page_table(PT_userAddress);

	// 1f) Complete initialization of address space with set_user_pages
	set_user_pages(PT_userAddress);

	// 1g) Map Directory and both page tables in the system page table
	set_ss_pag(PT_systemAddress, Dir, Dir, 0);
	set_ss_pag(PT_systemAddress, PT_system, PT_system, 0);
	set_ss_pag(PT_systemAddress, PT_user, PT_user, 0);

	// 1h) Assign System page table to first directory entry
	DirAddress[0].entry = 0;
	DirAddress[0].bits.pbase_addr = PT_system;
	DirAddress[0].bits.rw = 1;
	DirAddress[0].bits.present = 1;

	// 1i) Assign User page table to second directory entry with user permissions
	DirAddress[1].entry = 0;
	DirAddress[1].bits.pbase_addr = PT_user;
	DirAddress[1].bits.rw = 1;
	DirAddress[1].bits.present = 1;
	DirAddress[1].bits.user = 1;


	// PASO 2: Allocate a new struct task_struct
	struct list_head *first = list_first(&freequeue);
	list_del(first);
	struct task_struct *t = list_head_to_task_struct(first);


	// PASO 3: Map PCB in the system page table
	int PCB_frame = ((unsigned int)t) >> 12;
	set_ss_pag(PT_systemAddress, PCB_frame, PCB_frame, 0);


	// PASO 4: Assign PID 1 to the process
	t->PID = 1;
	t->quantum = 10;
	remaining_ticks = t->quantum;


	// PASO 5: Update the TSS to point to the new task system stack
	union task_union *tu = (union task_union *)t;
	tss.esp0 = (DWord)&(tu->stack[KERNEL_STACK_SIZE]);
	writeMSR(0x175, tss.esp0);

	// Initialize kernel_esp for use in fork()
	t->kernel_esp = (int)&(tu->stack[KERNEL_STACK_SIZE]);


	// PASO 6: Initialize dir_pages_baseAddr with the new directory
	t->dir_pages_baseAddr = DirAddress;


	// PASO 7: Set its page directory as the current page directory
	set_cr3(t->dir_pages_baseAddr);


	// PASO 8: Define global init_task and initialize it to this init PCB
	init_task = t;

}


void init_sched()
{
    INIT_LIST_HEAD(&freequeue);
    INIT_LIST_HEAD(&readyqueue);
    
    for (int i = 2; i < NR_TASKS; i++) {
        task[i].task.PID = -1; // Opcional: marquem com a no usat
        list_add_tail(&(task[i].task.list), &freequeue);
    }
}

void inner_task_switch(union task_union *new) {
	// 1)
	tss.esp0 = (DWord) &(new->stack[KERNEL_STACK_SIZE]); 
	writeMSR(0x175, (DWord) &(new->stack[KERNEL_STACK_SIZE])); 

	// 2)
	set_cr3(new->task.dir_pages_baseAddr);

	switch_stack(&(current()->kernel_esp), &(new->task.kernel_esp));
}

void update_sched_data_rr(void)
{
	if (current() != idle_task) {
		remaining_ticks--;
	}
}

int needs_sched_rr(void)
{
	if (!list_empty(&readyqueue)) {
		if (current() == idle_task) {
			return 1;
		}
		if (remaining_ticks <= 0) {
			return 1;
		}
	}
	return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
	if (dst_queue == NULL) {
		return;
	}

	list_add_tail(&t->list, dst_queue);
}

void sched_next_rr(void)
{
	if (list_empty(&readyqueue)) {
		return;
	}

	struct task_struct *next = list_head_to_task_struct(list_first(&readyqueue));
	list_del(&next->list);

	update_process_state_rr(next, NULL);
	remaining_ticks = get_quantum(next);
	task_switch((union task_union *) next);
}

void schedule(void)
{
	update_sched_data_rr();

	if (!needs_sched_rr()) {
		return;
	}

	struct task_struct *cur = current();

	if (cur != idle_task) {
		update_process_state_rr(cur, &readyqueue);
	}

	sched_next_rr();
}


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t)
{
       return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t)
{
       return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr[1].bits.pbase_addr))<<12);
}

struct task_struct * list_head_to_task_struct(struct list_head *l) {
	return (struct task_struct *)((unsigned int)l & 0xFFFFF000);
}

int get_quantum(struct task_struct *t) {
    return t->quantum;
}

void set_quantum(struct task_struct *t, int new_quantum) {
    t->quantum = new_quantum;
}
