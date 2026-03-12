#include <asm.h>
ENTRY(writeMSR)
    pushl %ebp
    movl %esp, %ebp
    
    movl 8(%ebp), %ecx    // Primer parámetro: número de registro MSR
    movl 12(%ebp), %eax   // Segundo parámetro: valor (parte baja)
    movl $0, %edx         // Ponemos EDX a 0 (parte alta) porque no usamos 64 bits de RAM
    
    wrmsr                 // Escribe edx:eax en el MSR indicado por ecx
    
    popl %ebp
    ret

