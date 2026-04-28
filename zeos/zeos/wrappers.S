#include <asm.h>
.extern int errno

ENTRY(read)
    pushl %ebp
    movl %esp, %ebp

    pushl %ebx
    movl 8(%ebp), %edx   // buffer -> EDX
    movl 12(%ebp), %ecx  // maxchars -> ECX

    movl $3, %eax        // 3 = syscall read

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

ENTRY(write)
    pushl %ebp
    movl %esp, %ebp
    
    // Pasamos parámetros de la pila a los registros como pide el profesor (y SAVE_ALL)
    pushl %ebx          // Salvar EBX según convención de C
    movl 8(%ebp), %edx  // fd   -> EDX
    movl 12(%ebp), %ecx // buff -> ECX
    movl 16(%ebp), %ebx // size -> EBX
    
    movl $4, %eax       // 4 = syscall de write
    
    // Truco del sysenter: fingir una llamada para guardar el retorno
    pushl %ecx          // Salvamos el registro
    pushl %edx
    
    // Fingimos el EBP para el retorno de sysenter
    pushl $return_from_sysenter 
    pushl %ebp          // Fake dynamic link
    movl %esp, %ebp     
    
    sysenter            // ¡Saltamos al sistema!

return_from_sysenter:
    popl %ebp           // Deshacemos el fake link
    addl $4, %esp       // Limpiamos la dirección de retorno
    
    popl %edx
    popl %ecx
    popl %ebx           // Restaurar EBX original
    
    cmpl $0, %eax       // Comparamos lo que devolvió sys_write con 0
    jge fin_write       // Si es Mayor o Igual (Jump if Greater or Equal), todo fue bien, saltamos al final

    // Si llegamos aquí, EAX es negativo (hubo error)
    negl %eax           // Le cambiamos el signo (ej: de -14 pasa a 14)
    movl %eax, errno    // Guardamos el 14 en la variable global errno
    movl $-1, %eax      // Sobreescribimos EAX con -1 (lo que devolverá la función write)
    
fin_write:
    popl %ebp
    ret

ENTRY(gettime)
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    
    movl $10, %eax      // 10 = syscall de gettime
    
    // Salvamos los registros que sysenter y sysexit machacan
    pushl %ecx
    pushl %edx
    
    // Fingimos el entorno para el retorno
    pushl $return_from_gettime
    pushl %ebp
    movl %esp, %ebp
    
    sysenter            // ¡Al kernel!

return_from_gettime:
    popl %ebp
    addl $4, %esp       // Limpiamos la dir de retorno
    
    popl %edx
    popl %ecx
    
    // gettime() nunca da error (nunca devuelve negativo), 
    // así que no hace falta gestionar el 'errno' aquí.
    // Devolvemos lo que haya en EAX directamente.
    
    popl %ebx
    popl %ebp
    ret


ENTRY(getpid)
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

ENTRY(fork)
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

ENTRY(exit)
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

ENTRY(block)
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

ENTRY(unblock)
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


