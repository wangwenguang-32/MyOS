.global system_call , sys_fork
system_call: 
        push  %ds
        push  %es
        push  %fs
        pushl %ebp
        pushl %edi
        pushl %esi
        pushl %edx
        pushl %ecx
        pushl %ebx
        pushl %eax
        cld
        movw $0x10, %dx
        movw %dx, %ds
        movw %dx, %es
        cmpl $5, %eax
        jae .Linvalid_syscall
        movl system_func_table(,%eax,4), %edx
        testl %edx, %edx
        jz .Linvalid_syscall
        call *%edx
        jmp .Lsyscall_result
.Linvalid_syscall:
        movl $-38, %eax
.Lsyscall_result:
        movl %eax, (%esp)

.global fork_return
fork_return:
        popl %eax
        popl %ebx
        popl %ecx
        popl %edx
        popl %esi
        popl %edi
        popl %ebp
        pop  %fs
        pop  %es
        pop  %ds
        iret


sys_fork:
        leal 4(%esp), %eax
        pushl %eax
        call do_fork
        addl $4, %esp
        ret
