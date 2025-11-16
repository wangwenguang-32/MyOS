.text
.global division_error
.global keyboard_isr
.global page_fault
.global general_protection
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
        call %eax
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
        pushl %eax
        call %ebx
        addl $4,%esp
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


/* 定时器中断处理 */
.global timer_isr
timer_isr:
        pushl %eax
        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %esi
        pushl %edi
        pushl %ebp
        push %ds
        push %es
        push %fs
        call timer_interrupt_handler
        cmpl $1,%eax
        je skip_schedule
        movl %esp,%ebx
        pushl %ebx
        call schedule
        addl $4,%esp
        movl %eax,%esp
  skip_schedule:
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