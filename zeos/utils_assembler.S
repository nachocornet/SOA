#include <asm.h>
#include <segment.h>

ENTRY(writeMSR)
    pushl %ebp
    movl %esp, %ebp
    
    movl 8(%ebp), %ecx    // Primer parámetro: número de registro MSR
    movl 12(%ebp), %eax   // Segundo parámetro: valor (parte baja)
    movl $0, %edx         // Ponemos EDX a 0 (parte alta) porque no usamos 64 bits de RAM
    
    wrmsr                 // Escribe edx:eax en el MSR indicado por ecx
    
    popl %ebp
    ret


ENTRY(task_switch)
	# void task_switch(union task_union *new)
	push %ebp 
	mov %esp, %ebp 

	# 1 
	push %esi
	push %edi
	push %ebx

    # 2
	push 8(%ebp) 
    # void inner_task_switch(union task_union *new)
	call inner_task_switch
	addl $4, %esp

    # 3
	pop %ebx
	pop %edi
	pop %esi

	mov %ebp,%esp
	pop %ebp 
	ret


ENTRY(switch_stack)
    push %ebp
    movl %esp, %ebp

    # old_ebp --> old_task->task.kernel_esp
    movl 8(%ebp), %eax
    movl %ebp, (%eax)       // current()->task.kernel_esp = ebp

    movl 12(%ebp), %eax 
    movl (%eax), %esp

    popl %ebp

    ret

ENTRY(get_esp)
    movl %esp, %eax
    ret

ENTRY(get_ebp)
    movl %ebp, %eax
    ret

