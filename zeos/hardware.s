# 0 "hardware.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "hardware.S"
# 1 "include/asm.h" 1
# 2 "hardware.S" 2


.globl get_eflags; .type get_eflags, @function; .align 0; get_eflags:
    pushf
    popl %eax
    ret



.globl set_eflags; .type set_eflags, @function; .align 0; set_eflags:
    pushl $0
    popfl
    ret



.globl set_idt_reg; .type set_idt_reg, @function; .align 0; set_idt_reg:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    lidtl (%eax)
    popl %ebp
    ret



.globl set_gdt_reg; .type set_gdt_reg, @function; .align 0; set_gdt_reg:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    lgdtl (%eax)
    popl %ebp
    ret



.globl set_ldt_reg; .type set_ldt_reg, @function; .align 0; set_ldt_reg:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    lldtw %ax
    popl %ebp
    ret



.globl set_task_reg; .type set_task_reg, @function; .align 0; set_task_reg:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    ltrw %ax
    popl %ebp
    ret



.globl return_gate; .type return_gate, @function; .align 0; return_gate:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    movw %ax, %es
    movw %ax, %ds
    movl 16(%ebp), %eax
    subl $16, %eax
    movl (p_rdtr), %ecx
    movl (%ecx), %ecx
    movl %ecx, 12(%eax)
    pushl 12(%ebp)
    pushl %eax
    pushl 20(%ebp)
    pushl 24(%ebp)
    lret
# 96 "hardware.S"
.globl enable_int; .type enable_int, @function; .align 0; enable_int:
    pushl %ebp
    movl %esp, %ebp
    movb $0xfc, %al
    outb %al, $0x21
    call delay
    sti
    pop %ebp
    ret
delay:
    jmp a
a:
    ret



.globl inb; .type inb, @function; .align 0; inb:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %edx
    inb (%dx), %al
    popl %ebp
    ret



.globl bochs_out; .type bochs_out, @function; .align 0; bochs_out:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    outb %al, $0xe9
    popl %ebp
    ret



.globl set_cr3; .type set_cr3, @function; .align 0; set_cr3:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    movl %eax, %cr3
    popl %ebp
    ret



.globl read_cr0; .type read_cr0, @function; .align 0; read_cr0:
    movl %cr0, %eax
    ret



.globl write_cr0; .type write_cr0, @function; .align 0; write_cr0:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    movl %eax, %cr0
    popl %ebp
    ret



.globl __sti; .type __sti, @function; .align 0; __sti:
    sti
    ret



.globl current; .type current, @function; .align 0; current:
    movl %esp, %eax
    andl $0xFFFFF000, %eax
    ret



.globl set_seg_regs; .type set_seg_regs, @function; .align 0; set_seg_regs:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    cld
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movl 12(%ebp), %eax
    movl 16(%ebp), %ecx
    subl $0x10, %ecx
    movl 4(%ebp), %edx
    movw %ax, %ss
    movl %ecx, %esp
    pushl %edx
    ret
