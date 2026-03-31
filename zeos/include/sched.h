/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>

#define KERNEL_STACK_SIZE	1024
#define NR_TASKS		30

struct task_struct {
  int PID;            /* Process ID. This MUST be the first field of the struct. */
  page_table_entry * dir_pages_baseAddr;

  struct list_head list;
  int kernel_esp;
  int quantum;
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};


#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

extern char initial_stack[KERNEL_STACK_SIZE];
#define INITIAL_ESP             (DWord) &initial_stack[KERNEL_STACK_SIZE]

extern union task_union task[NR_TASKS];
extern struct list_head readyqueue;
extern struct list_head freequeue;
extern struct task_struct *idle_task;
extern struct task_struct *init_task;

/* Inicialitza les dades del proces inicial */
void allocate_DIR(struct task_struct *t);

void init_task1(void);

void init_idle(void);

void init_sched(void);

void update_sched_data_rr(void);
int needs_sched_rr(void);
void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue);
void sched_next_rr(void);
void schedule(void);
int get_quantum(struct task_struct *t);
void set_quantum(struct task_struct *t, int new_quantum);

struct task_struct * current();

void task_switch(union task_union *new);

void inner_task_switch(union task_union *new);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

struct task_struct * list_head_to_task_struct(struct list_head *l);

#endif  /* __SCHED_H__ */
