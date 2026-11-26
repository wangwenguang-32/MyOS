.text
.code32
.globl swtch
.type swtch, @function
# void swtch(task_context_t **old, task_context_t **next)
# Stack layout matches task_context_t: edi, esi, ebx, ebp, return eip.
swtch:
    movl 4(%esp), %eax
    movl 8(%esp), %edx
    pushl %ebp
    pushl %ebx
    pushl %esi
    pushl %edi
    movl %esp, (%eax)
    movl (%edx), %esp
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    ret
.size swtch, .-swtch
