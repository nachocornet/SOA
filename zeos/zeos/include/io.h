/*
 * io.h - Definició de l'entrada/sortida per pantalla en mode sistema
 */

#ifndef __IO_H__
#define __IO_H__

#include <types.h>

/** Screen functions **/
/**********************/

void printc(char c);
void printc_xy(Byte x, Byte y, char c);
void printk(char *string);
void screen_set_pos(Byte x, Byte y);
void screen_set_color(Byte fg, Byte bg);
Byte screen_get_color(void);

#endif  /* __IO_H__ */
