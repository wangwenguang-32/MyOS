.global system_call
system_call:
        pushl $0
        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %esi
        pushl %edi
        pushl %ebp
        push %ds
        push %es
        push %fs
        pushl %edx
        pushl %ecx
        pushl %ebx
        call system_func_table(,%eax,4)
        addl $12,%esp
        pop %fs
        pop %es
        pop %ds
        popl %ebp
        popl %edi
        popl %esi
        popl %edx
        popl %ecx
        popl %ebx
        addl $4,%esp
        iret