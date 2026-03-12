/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#define LECTURA 0
#define ESCRIPTURA 1


int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

#include <io.h>
#include <system.h> 

int sys_write(int fd, char *buffer, int size) {
    int error;
    char sys_buffer[256]; 
    int bytes_left = size;
    int bytes_written = 0;

    error = check_fd(fd, ESCRIPTURA); 
    if (error < 0) return error; 
    if (buffer == 0) return -14; 
    if (size < 0) return -22;    
    error = access_ok(ESCRIPTURA, buffer, size);
    if (error < 0) return error;

    while (bytes_left > 0) {
        int chunk_size = (bytes_left > 256) ? 256 : bytes_left;
        
        copy_from_user(buffer + bytes_written, sys_buffer, chunk_size);
        
        sys_write_console(sys_buffer, chunk_size);
        
        bytes_written += chunk_size;
        bytes_left -= chunk_size;
    }

    return bytes_written; 
}

extern int zeos_ticks;

int sys_gettime()
{
    return zeos_ticks;
}


