#include <io.h>
#include <libc.h>
#include <utils.h>
#include <list.h>
#include <sched.h>

#define KEYBOARD_BUFFER_SIZE 16

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int keyboard_head = 0;
static int keyboard_tail = 0;
static int keyboard_items = 0;
static struct list_head keyboard_waitqueue;

extern void task_switch(union task_union *new);

int sys_write_console(char *buffer,int size)
{
  int i;
  
  for (i=0; i<size; i++)
    printc(buffer[i]);
  
  return size;
}

void keyboard_buffer_init(void)
{
  keyboard_head = 0;
  keyboard_tail = 0;
  keyboard_items = 0;
}

void keyboard_waitqueue_init(void)
{
  INIT_LIST_HEAD(&keyboard_waitqueue);
}

int keyboard_buffer_push(char c)
{
  if (keyboard_items >= KEYBOARD_BUFFER_SIZE) return -1;

  keyboard_buffer[keyboard_head] = c;
  keyboard_head = (keyboard_head + 1) % KEYBOARD_BUFFER_SIZE;
  keyboard_items++;
  return 0;
}

int keyboard_buffer_pop(char *c)
{
  if (keyboard_items == 0 || c == 0) return -1;

  *c = keyboard_buffer[keyboard_tail];
  keyboard_tail = (keyboard_tail + 1) % KEYBOARD_BUFFER_SIZE;
  keyboard_items--;
  return 0;
}

int keyboard_buffer_count(void)
{
  return keyboard_items;
}

void keyboard_block_current_reader(void)
{
  struct task_struct *p = current();

  if (p == idle_task) return;

  update_process_state_rr(p, &keyboard_waitqueue);

  if (!list_empty(&readyqueue)) {
    sched_next_rr();
  } else {
    task_switch((union task_union *)idle_task);
  }
    
}

void keyboard_wake_one_reader(void)
{
  if (list_empty(&keyboard_waitqueue)) return;

  struct task_struct *p = list_head_to_task_struct(list_first(&keyboard_waitqueue));
  update_process_state_rr(p, &readyqueue);
}

void keyboard_buffer_debug_dump(void)
{
  int i;
  int pos = keyboard_tail;
  char num[8];

  printk("\n[KBD BUFFER] count=");
  itoa(keyboard_items, num);
  printk(num);
  printk(" head=");
  itoa(keyboard_head, num);
  printk(num);
  printk(" tail=");
  itoa(keyboard_tail, num);
  printk(num);
  printk(" | ");
  if (keyboard_items == 0) {
    printk("<empty>\n");
    return;
  }

  for (i = 0; i < keyboard_items; ++i) {
    printc(keyboard_buffer[pos]);
    pos = (pos + 1) % KEYBOARD_BUFFER_SIZE;
  }
  printk("\n");
}
