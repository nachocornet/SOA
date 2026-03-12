#include <libc.h>

char buff[24];

int pid;


extern int addASM(int a, int b);

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  int resultado = write(1, "Este write va a dar error porque el tamaño es negativo\n", -10);

    if (resultado < 0) {
        perror(); 
    } else {
        write(1, "Escritura correcta\n", 19);
    }


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

  write(1, "Ahora va a haber una fallo de página porque se va a acceder a una dirección no mapeada\n", 92);
  int *ptr = (int *)0x0;
  int valor = *ptr;
}
