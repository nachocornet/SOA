#include <libc.h>

char buff[24];

int pid;


extern int addASM(int a, int b);

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */



    int tiempo_incial = gettime();
    int cont = 0;
  while (cont < 10){ 
    int tiempo_actual = gettime();
    if (tiempo_actual - tiempo_incial > 100) {
        write(1, "Tiempo transcurrido: ", 21);
        itoa(tiempo_actual - tiempo_incial, buff);
        write(1, buff, strlen(buff));
        write(1, "\n", 1);
        itoa(tiempo_actual, buff);
        write(1, "Tiempo actual: ", 15);
        write(1, buff, strlen(buff));
        write(1, "\n", 1);
        tiempo_incial = tiempo_actual;
        cont++;
    }
  }

  int pid_fork, myPID;
  write(1, "Llamando a fork...\n", 20);
  pid_fork = fork();

  if (pid_fork == 0) {
    write(1, "Soy el proceso hijo\n", 21);
    myPID = getpid();
    char pidStr[10];
    itoa(myPID, pidStr);
    write(1, "PID del proceso hijo: ", 22);
    write(1, pidStr, strlen(pidStr));
    write(1, "\n", 1);
  } else if (pid_fork > 0) {
    write(1, "Soy el proceso padre\n", 21);
    int pidHijo = pid_fork;
    write(1, "PID del proceso hijo (desde el padre): ", 37);
    itoa(pidHijo, buff);
    write(1, buff, strlen(buff));
    write(1, "\n", 1);
    myPID = getpid();
    char pidStr[10];
    itoa(myPID, pidStr);
    write(1, "PID del proceso padre: ", 22);
    write(1, pidStr, strlen(pidStr));
    write(1, "\n", 1);
  } else {
    write(1, "Error al crear el proceso hijo\n", 31);
  }

  /* make sure main does not return.  In this tiny OS the kernel
     does not set up a return address for the first user function, so
     falling off the end of main will pop a garbage value and jump
     somewhere inside the image (0xEBFE is a repeat of "jmp -2"!) which
     triggers the invalid‑opcode fault you were seeing.  Instead of
     returning we simply spin forever or you can explicitly call a
     termination syscall if one is provided by your kernel. */

  /* never return from user code */
  for(;;) {
    for(int i = 0;i < 1000000;i++) {}
      int pidN = getpid();
    char pidStr[10];
    itoa(pidN, pidStr);
    write(1, "PID del proceso: ", 17);
    write(1, pidStr, strlen(pidStr));
    write(1, "\n", 1);
  }
}
