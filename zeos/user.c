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

  /* make sure main does not return.  In this tiny OS the kernel
     does not set up a return address for the first user function, so
     falling off the end of main will pop a garbage value and jump
     somewhere inside the image (0xEBFE is a repeat of "jmp -2"!) which
     triggers the invalid‑opcode fault you were seeing.  Instead of
     returning we simply spin forever or you can explicitly call a
     termination syscall if one is provided by your kernel. */

  /* never return from user code */
  for(;;) {}
}
