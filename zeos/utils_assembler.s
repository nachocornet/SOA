# 0 "utils_assembler.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "utils_assembler.S"
# 1 "include/asm.h" 1
# 2 "utils_assembler.S" 2
# 1 "include/segment.h" 1
# 3 "utils_assembler.S" 2

.globl writeMSR; .type writeMSR, @function; .align 0; writeMSR:
    pushl %ebp
    movl %esp, %ebp

    movl 8(%ebp), %ecx
    movl 12(%ebp), %eax
    movl $0, %edx

    wrmsr

    popl %ebp
    ret


.globl task_switch; .type task_switch, @function; .align 0; task_switch:
 # void task_switch(union task_union *new)
 push %ebp
 mov %esp, %ebp

 # 1
 push %ebx
 push %esi
 push %edi

    # 2
 push 8(%ebp)
    # void inner_task_switch(union task_union *new)
 call inner_task_switch
 addl $4, %esp

    # 3
 pop %edi
 pop %esi
 pop %ebx

 mov %ebp,%esp
 pop %ebp
 ret


.globl switch_stack; .type switch_stack, @function; .align 0; switch_stack:
    push %ebp
    movl %esp, %ebp

    # old_ebp --> old_task->task.kernel_esp
    movl 8(%ebp), %eax
    movl %ebp, (%eax)

    movl 12(%ebp), %esp

    popl %ebp

    ret
