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

int write(int fd, char *buffer, int size);

int gettime(void);

#endif  /* __LIBC_H__ */
