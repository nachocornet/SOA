/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno;

void perror(void) {
    char *error_msg;

    // Traducimos el código numérico a texto legible
    switch(errno) {
        case 9:  // EBADF
            error_msg = "Bad file descriptor"; 
            break;
        case 14: // EFAULT
            error_msg = "Bad address (Puntero nulo o invalido)"; 
            break;
        case 22: // EINVAL
            error_msg = "Invalid argument (Tamano negativo)"; 
            break;
        case 38: // ENOSYS
            error_msg = "Function not implemented"; 
            break;
        default:
            error_msg = "Unknown error"; 
            break;
    }
    write(1, "\nError: ", 8);
    write(1, error_msg, strlen(error_msg)); 
    write(1, "\n", 1);
}

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

