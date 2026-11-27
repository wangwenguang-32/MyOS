#include <process.h>
#include <scheduler.h>
#include <kernel.h>
#include <gdt.h>
#include <mm_lock.h>
#include <printf.h>
extern struct tss_struct tss_globel;

/* UP round-robin scheduler with a dedicated idle fallback. */
void schedule(void)
{
    uint32_t flags = mm_irq_save();
    task_t *prev = current, *next;
    if (!prev || !init_task0) { mm_irq_restore(flags); return; }
    if (list_empty(&ready_task_head)) {
        if (prev->state == TASK_RUNNING && prev != init_task0) {
            prev->time_slice = PROCESS_TIME_SLICE;
            mm_irq_restore(flags);
            return;
        }
        next = init_task0;
    } else next = list_entry(ready_task_head.next, task_t, ready_node);
    if (next == prev || (next != init_task0 && next->state != TASK_READY) || !next->context ||
        !next->mm || !next->mm->pgd) {
        mm_irq_restore(flags);
        return;
    }
    if (next != init_task0) {
        list_del(&next->ready_node);
        INIT_LIST_HEAD(&next->ready_node);
    }
    if (prev->state == TASK_RUNNING && prev != init_task0) {
        prev->state = TASK_READY;
        list_add_tail(&prev->ready_node, &ready_task_head);
    }
    if (prev == init_task0) prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    next->time_slice = PROCESS_TIME_SLICE;
    next->on_cpu = 0; /* CPU number, not a boolean. */
    tss_globel.esp0 = (uint32_t)next + PAGE_SIZE;
    tss_globel.ss0 = SEG_KERNEL_DATA;
    if (!prev->mm || !prev->mm->pgd) {
        printf("[sched] invalid previous address space\n");
        for (;;) __asm__ volatile("cli; hlt");
    }
    if (prev->state == TASK_UNUSED) {
        uint32_t stack, base;
        __asm__ volatile("movl %%esp, %0" : "=r"(stack));
        __asm__ volatile("movl %%ebp, %0" : "=r"(base));
        printf("[sched] prev=0x%x next=0x%x pgd=0x%x context=0x%x entry=0x%x\n",
               (uint32_t)prev, (uint32_t)next, (uint32_t)next->mm->pgd,
               (uint32_t)next->context, next->context->eip);
        printf("[sched] ESP=0x%x EBP=0x%x\n", stack, base);
    }
    if (prev->mm->pgd != next->mm->pgd) {
        uint32_t physical = (uint32_t)next->mm->pgd - PAGE_OFFSET;
        __asm__ volatile("movl %0, %%cr3" :: "r"(physical) : "memory");
    }
    current = next;
    swtch(&prev->context, &next->context);
    /* Runs when this task is selected again, on its own saved stack. */
    mm_irq_restore(flags);
}
