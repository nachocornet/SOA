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
        case 12:
            error_msg = "Out of memory"; 
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

/* FPS counter */
static int fps_frames = 0;
static int fps_last_ticks = -1;
static int fps_current = 0;

void fps_update(void)
{
    int current_ticks = gettime();
    fps_frames++;
    
    /* Initialize on first call */
    if (fps_last_ticks == -1) {
        fps_last_ticks = current_ticks;
    }
    
    /* Update FPS every 100ms for more responsiveness */
    if (current_ticks - fps_last_ticks >= 100) {
        fps_current = (fps_frames * 1000) / (current_ticks - fps_last_ticks);
        fps_frames = 0;
        fps_last_ticks = current_ticks;
    }
}

void display_fps(void)
{
    char fps_str[16];
    
    /* Move to top-left corner and overwrite FPS display */
    gotoxy(0, 0);
    
    /* Write FPS counter */
    write(1, "FPS: ", 5);
    itoa(fps_current, fps_str);
    write(1, fps_str, strlen(fps_str));
    
    /* Pad with spaces to clear old numbers */
    write(1, "    ", 4);
    
    /* Move back to (0, 1) to avoid interfering with main output */
    gotoxy(0, 1);
}

