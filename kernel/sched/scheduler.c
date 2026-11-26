#include <process.h>
#include <scheduler.h>
#include <kernel.h>
#include <gdt.h>
#include <mm_lock.h>
extern struct tss_struct tss_globel;

/* UP scheduler: callers cannot sleep without a runnable fallback task. */
void schedule(void)
{
    uint32_t flags = mm_irq_save();
    task_t *prev = current, *next;
    if (!prev || list_empty(&ready_task_head)) {
        if (prev && prev->state == TASK_RUNNING) prev->time_slice = PROCESS_TIME_SLICE;
        mm_irq_restore(flags);
        return;
    }
    next = list_entry(ready_task_head.next, task_t, ready_node);
    if (next == prev || next->state != TASK_READY || !next->context ||
        !next->mm || !next->mm->pgd) {
        mm_irq_restore(flags);
        return;
    }
    list_del(&next->ready_node);
    INIT_LIST_HEAD(&next->ready_node);
    if (prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
        list_add_tail(&prev->ready_node, &ready_task_head);
    }
    next->state = TASK_RUNNING;
    next->time_slice = PROCESS_TIME_SLICE;
    next->on_cpu = 0; /* CPU number, not a boolean. */
    tss_globel.esp0 = (uint32_t)next + PAGE_SIZE;
    tss_globel.ss0 = SEG_KERNEL_DATA;
    if (prev->mm->pgd != next->mm->pgd) {
        uint32_t physical = (uint32_t)next->mm->pgd - PAGE_OFFSET;
        __asm__ volatile("movl %0, %%cr3" :: "r"(physical) : "memory");
    }
    current = next;
    swtch(&prev->context, &next->context);
    /* Runs when this task is selected again, on its own saved stack. */
    mm_irq_restore(flags);
}
