# 0 "wrappers.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "wrappers.S"
# 1 "include/asm.h" 1
# 2 "wrappers.S" 2
.extern int errno
.globl write; .type write, @function; .align 0; write:
    pushl %ebp
    movl %esp, %ebp


    pushl %ebx
    movl 8(%ebp), %edx
    movl 12(%ebp), %ecx
    movl 16(%ebp), %ebx

    movl $4, %eax


    pushl %ecx
    pushl %edx


    pushl $return_from_sysenter
    pushl %ebp
    movl %esp, %ebp

    sysenter

return_from_sysenter:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx
    popl %ebx

    cmpl $0, %eax
    jge fin_write


    negl %eax
    movl %eax, errno
    movl $-1, %eax

fin_write:
    popl %ebp
    ret

.globl gettime; .type gettime, @function; .align 0; gettime:
    pushl %ebp
    movl %esp, %ebp

    movl $10, %eax


    pushl %ecx
    pushl %edx


    pushl $return_from_gettime
    pushl %ebp
    movl %esp, %ebp

    sysenter

return_from_gettime:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx





    popl %ebp
    ret
