.text
.global division_error
.global keyboard_isr
.global page_fault
.global general_protection
.global peserved_0
.global about_double

no_error_code:
        xchgl %eax,(%esp)
        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %esi
        pushl %edi
        pushl %ebp
        push %ds
        push %es
        push %fs
        cld
        movw $0x10, %dx
        movw %dx, %ds
        movw %dx, %es
        call *%eax
        pop %fs
        pop %es
        pop %ds
        popl %ebp
        popl %edi
        popl %esi
        popl %edx
        popl %ecx
        popl %ebx
        popl %eax
        iret


error_code:
        xchgl %eax,4(%esp)
        xchgl %ebx,(%esp)
        pushl %ecx
        pushl %edx
        pushl %esi
        pushl %edi
        pushl %ebp
        push %ds
        push %es
        push %fs
        leal 40(%esp), %edx
        pushl %edx
        pushl %eax
        cld
        movw $0x10, %dx
        movw %dx, %ds
        movw %dx, %es
        call *%ebx
        addl $8,%esp
        pop %fs
        pop %es
        pop %ds
        popl %ebp
        popl %edi
        popl %esi
        popl %edx
        popl %ecx
        popl %ebx
        popl %eax
        iret


division_error:
        push  $do_division_error
        jmp no_error_code


keyboard_isr:
        push  $keyboard_interrupt_handler
        jmp no_error_code

page_fault:
        push $do_page_fault
        jmp error_code

general_protection:
        push $do_general_protection
        jmp error_code


save_all_regs:
        pushl %eax
        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %esi
        pushl %edi
        pushl %ebp
        push  %ds
        push  %es
        push  %fs
        ret

restore_all_regs:
        pop  %fs
        pop  %es
        pop  %ds
        popl %ebp
        popl %edi
        popl %esi
        popl %edx
        popl %ecx
        popl %ebx
        popl %eax
        ret

.global timer_interrupt
timer_interrupt:
        push $timer_interrupt_handler
        jmp no_error_code



peserved_0:
        push $do_peserved_0
        jmp no_error_code

about_double:
        push $do_about_double
        jmp error_code


