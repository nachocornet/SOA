#include <io.h>
#include <libc.h>
#include <utils.h>
#include <list.h>

#define KEYBOARD_BUFFER_SIZE 16

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int keyboard_head = 0;
static int keyboard_tail = 0;
static int keyboard_items = 0;

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
