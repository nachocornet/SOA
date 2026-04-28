# 0 "wrappers.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "wrappers.S"
# 1 "include/asm.h" 1
# 2 "wrappers.S" 2
.extern int errno

.globl read; .type read, @function; .align 0; read:
    pushl %ebp
    movl %esp, %ebp

    pushl %ebx
    movl 8(%ebp), %edx
    movl 12(%ebp), %ecx

    movl $3, %eax

    pushl %ecx
    pushl %edx

    pushl $return_from_read
    pushl %ebp
    movl %esp, %ebp

    sysenter

return_from_read:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx
    popl %ebx

    cmpl $0, %eax
    jge fin_read

    negl %eax
    movl %eax, errno
    movl $-1, %eax

fin_read:
    popl %ebp
    ret

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
    pushl %ebx

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





    popl %ebx
    popl %ebp
    ret


.globl getpid; .type getpid, @function; .align 0; getpid:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx

    pushl %ecx
    pushl %edx

    movl $20, %eax

    pushl $return_getpid
    pushl %ebp

    movl %esp, %ebp
    sysenter

return_getpid:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge fi_getpid

    negl %eax
    movl %eax, errno
    movl $-1, %eax

fi_getpid:
    popl %ebx
    popl %ebp
    ret

.globl fork; .type fork, @function; .align 0; fork:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx

    pushl %ecx
    pushl %edx

    movl $2, %eax

    pushl $return_fork
    pushl %ebp
    movl %esp, %ebp
    sysenter

return_fork:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge fi_fork

    negl %eax
    movl %eax, errno
    movl $-1, %eax

fi_fork:
    popl %ebx
    popl %ebp
    ret

.globl exit; .type exit, @function; .align 0; exit:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx

    pushl %ecx
    pushl %edx

    movl $1, %eax

    pushl $return_exit

    pushl %ebp
    movl %esp, %ebp
    sysenter

return_exit:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge fi_exit

    negl %eax
    movl %eax, errno
    movl $-1, %eax

fi_exit:
    popl %ebx
    popl %ebp
    ret

.globl block; .type block, @function; .align 0; block:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx

    pushl %ecx
    pushl %edx

    movl $21, %eax

    pushl $return_block
    pushl %ebp
    movl %esp, %ebp
    sysenter

return_block:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx

    popl %ebx
    popl %ebp
    ret

.globl unblock; .type unblock, @function; .align 0; unblock:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx

    pushl %ecx
    pushl %edx

    movl 8(%ebp), %edx
    movl $22, %eax

    pushl $return_unblock
    pushl %ebp
    movl %esp, %ebp
    sysenter

return_unblock:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge fi_unblock

    negl %eax
    movl %eax, errno
    movl $-1, %eax

fi_unblock:
    popl %ebx
    popl %ebp
    ret
