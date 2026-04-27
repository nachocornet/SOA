/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__
extern int errno;

void perror(void);

void itoa(int a, char *b);

int strlen(char *a);

int read(char *buffer, int maxchars);

int write(int fd, char *buffer, int size);

int gotoxy(int x, int y);

int set_color(int fg, int bg);

int gettime(void);

int getpid(void);

int fork(void);

void exit(void);

void block(void);

int unblock(int pid);

#endif  /* __LIBC_H__ */
