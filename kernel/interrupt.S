.text
.global division_error
.global keyboard_isr
.global page_fault
.global general_protection
.global peserved_0
.global about_double
.global int_1
.global int_2
.global int_3
.global int_4
.global int_5
.global int_6
.global int_7
.global int_A
.global int_B
.global int_C
.global int_F
.global int_10
.global int_11
.global int_12
.global int_13
.global int_14
.global int_15


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
.global context_switch
context_switch:
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


peserved_0:
        push $do_peserved_0
        jmp no_error_code

about_double:
        push $do_about_double
        jmp error_code


int_1:
        push $do_int1
        jmp no_error_code

int_2:
        push $do_int2
        jmp no_error_code

int_3:
        push $do_int3
        jmp no_error_code

int_4:
        push $do_int4
        jmp no_error_code

int_5:
        push $do_int5
        jmp no_error_code

int_6:
        push $do_int6
        jmp no_error_code

int_7:
        push $do_int7
        jmp no_error_code


int_A:
        push $do_intA
        jmp error_code

int_B:
        push $do_intB
        jmp error_code

int_C:
        push $do_intC
        jmp error_code

int_F:
        push $do_intF
        jmp no_error_code

int_10:
        push $do_int10
        jmp no_error_code

int_11:
        push $do_int11
        jmp error_code


int_12:
        push $do_int12
        jmp no_error_code

int_13:
        push $do_int13
        jmp no_error_code

int_14:
        push $do_int14
        jmp no_error_code

int_15:
        push $do_int15
        jmp no_error_code


