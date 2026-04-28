#include <asm.h>

ENTRY(addASM)
    pushl %ebp
    movl %esp, %ebp


    movl 0x8(%ebp), %edx
    movl 0xc(%ebp), %eax
    addl %edx, %eax

    pop %ebp
    ret