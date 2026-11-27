# Position-independent user program copied into a read-only user page.
.section .rodata.user_init,"a",@progbits
.code32
.global user_init_start, user_init_end
user_init_start:
    call 1f
1:  popl %esi
    addl $(message-1b), %esi
2:  movzbl (%esi), %ebx
    testl %ebx, %ebx
    jz 3f
    movl $3, %eax
    int $0x80
    incl %esi
    jmp 2b
3:  movl $4, %eax
    int $0x80
    jmp 3b
message:
    .asciz "[user init] PID 1 entered ring 3.\n"
user_init_end:

.section .text
.global enter_user_init
.type enter_user_init,@function
enter_user_init:
    cli
    movw $0x23, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    pushl $0x23
    pushl $0xbffffff0
    pushl $0x202
    pushl $0x1b
    pushl $0x08048000
    iret
