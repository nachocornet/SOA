/*
 * io.h - Definició de l'entrada/sortida per pantalla en mode sistema
 */

#ifndef __IO_H__
#define __IO_H__

#include <types.h>

/** Screen functions **/
/**********************/

void printc(char c);
int screen_gotoxy(int x, int y);
int screen_set_color(int fg, int bg);
void printc_xy(Byte x, Byte y, char c);
void printk(char *string);

#endif  /* __IO_H__ */
