#ifndef DEVICES_H__
#define  DEVICES_H__

int sys_write_console(char *buffer,int size);

void keyboard_buffer_init(void);
int keyboard_buffer_push(char c);
int keyboard_buffer_pop(char *c);
int keyboard_buffer_count(void);
void keyboard_buffer_debug_dump(void);
void keyboard_waitqueue_init(void);
void keyboard_block_current_reader(void);
void keyboard_wake_one_reader(void);
#endif /* DEVICES_H__*/
