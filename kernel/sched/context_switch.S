void swtch(struct context *old, struct context *new);
.globl swtch
swtch:
    movl 4(%esp), %eax   # eax = old指针（第一个参数）
    movl 8(%esp), %edx   # edx = new指针（第二个参数）

    pushl %ebp
    pushl %ebx
    pushl %esi
    pushl %edi

    movl %esp, (%eax)

    movl (%edx), %esp    # 把new进程保存的context里的栈指针加载到esp
    
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp

    ret
