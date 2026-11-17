#ifndef SCHEDULE_H
#define SCHEDULE_H

#define save_com_regs()    \
__asm__ __volatile__(  \
        "pushl %eax\n\t" \
        "pushl %ebx\n\t" \
        "pushl %ecx\n\t" \
        "pushl %edx\n\t" \
        "pushl %esi\n\t" \
        "pushl %edi\n\t" \
        "pushl %ebp\n\t" \
        "push  %ds \n\t" \
        "push  %es \n\t" \
        "push  %fs \n\t")


#define restore_com_regs() \
__asm__ __volatile__( \
        "pop  %fs\n\t"   \
        "pop  %es\n\t"   \
        "pop  %ds\n\t"   \
        "popl %ebp\n\t"  \
        "popl %edi\n\t"  \
        "popl %esi\n\t"  \
        "popl %edx\n\t"  \
        "popl %ecx\n\t"  \
        "popl %ebx\n\t"  \
        "popl %eax\n\t")



void schedule();


#endif