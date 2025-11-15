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


.global switch_to
switch_to:
        /* 保存当前进程的寄存器 */
        pushl %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
        
        /* 在切换栈之前获取参数 */
        movl 24(%esp), %eax      /* eax = next_esp (参数在栈上，跳过保存的寄存器) */
        movl 28(%esp), %ecx      /* ecx = next_cr3 */
        
        /* 切换到下一个进程的栈 */
        movl %eax, %esp          /* esp = next_esp */
        
        /* 切换页目录（CR3） */
        movl %ecx, %cr3          /* 切换页目录 */
        
        /* 恢复下一个进程的寄存器 */
        popl %edi
        popl %esi
        popl %ebx
        popl %ebp
        
        ret

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