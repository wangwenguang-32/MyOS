#include <process.h>
#include <scheduler.h>
#include <physical_memory.h>
#include <heap.h>
#include <kernel.h>
#include <gdt.h>
#include <pid.h>
#include <mm_lock.h>
#include <printf.h>

task_t *current;
task_t *init_task0;
task_t *init_process;
struct tss_struct tss_globel __attribute__((aligned(4)));
LIST_HEAD(ready_task_head);
LIST_HEAD(all_task_head);
LIST_HEAD(block_task_head);
LIST_HEAD(sleep_task_head);
static mm_struct_t kernel_mm;
static task_t boot_task;
extern void init_main(void);
extern int prepare_user_init(task_t *task);

static void idle_main(void)
{
    for (;;) {
        __asm__ volatile("cli" ::: "memory");
        if (!list_empty(&ready_task_head)) {
            schedule();
            __asm__ volatile("sti" ::: "memory");
        } else {
            /* Atomic enable-and-wait closes the lost-wakeup window. */
            __asm__ volatile("sti; hlt" ::: "memory");
        }
    }
}

static void kernel_task_entry(void)
{
    __asm__ volatile("sti" ::: "memory");
    if (current == init_task0) idle_main();
    else init_main();
    /* Retain a returned init as a blocked task; no unsafe stack reclamation. */
    for (;;) {
        __asm__ volatile("cli" ::: "memory");
        current->state = TASK_UNINTERRUPTIBLE;
        schedule();
    }
}

typedef char task_stack_room_check[(sizeof(task_t) + sizeof(task_context_t) + 1024 < PAGE_SIZE) ? 1 : -1];

static task_t *create_kernel_task(int32_t pid, sched_policy_t policy)
{
    uint32_t physical = alloc_page();
    task_t *task;
    task_context_t *context;
    if (!physical) return 0;
    task = (void *)(physical + PAGE_OFFSET);
    for (uint32_t i = 0; i < PAGE_SIZE; i++) ((uint8_t *)task)[i] = 0;
    task->pid = pid;
    task->ppid = pid ? 0 : -1;
    task->state = TASK_READY;
    task->policy = policy;
    task->time_slice = PROCESS_TIME_SLICE;
    task->mm = &kernel_mm;
    INIT_LIST_HEAD(&task->ready_node);
    INIT_LIST_HEAD(&task->all_tasks_node);
    INIT_LIST_HEAD(&task->block_node);
    INIT_LIST_HEAD(&task->sleep_node);
    /* ret enters with ESP = top-4, matching i386 C function entry alignment. */
    context = (void *)((uint32_t)task + PAGE_SIZE - 4 - sizeof(*context));
    context->eip = (uint32_t)kernel_task_entry;
    task->context = context;
    return task;
}

int init_task(void)
{
    int32_t pid;
    if (current) return 0;
    kernel_mm = (mm_struct_t){0};
    kernel_mm.pgd = _pdt;
    INIT_LIST_HEAD(&kernel_mm.vm_area_list);
    pid_allocator_init();
    init_task0 = create_kernel_task(0, SCHED_IDLE);
    if (!init_task0) return -1;
    pid = pid_alloc();
    if (pid != 1) { free_page((uint32_t)init_task0 - PAGE_OFFSET); init_task0 = 0; return -1; }
    init_process = create_kernel_task(pid, SCHED_NORMAL);
    if (!init_process) {
        pid_free(pid);
        free_page((uint32_t)init_task0 - PAGE_OFFSET);
        init_task0 = 0;
        return -1;
    }
    if (prepare_user_init(init_process)) {
        free_page((uint32_t)init_process - PAGE_OFFSET);
        free_page((uint32_t)init_task0 - PAGE_OFFSET);
        pid_free(pid);
        init_process = init_task0 = 0;
        return -1;
    }
    list_add_tail(&init_task0->all_tasks_node, &all_task_head);
    list_add_tail(&init_process->all_tasks_node, &all_task_head);
    list_add_tail(&init_process->ready_node, &ready_task_head);
    /* One-way bridge from boot stack to a real task stack. Never enqueue it. */
    boot_task = (task_t){0};
    boot_task.state = TASK_UNUSED;
    boot_task.mm = &kernel_mm;
    current = &boot_task;
    return 0;
}

int task_wake(task_t *task)
{
    uint32_t flags = mm_irq_save();
    if (!task || task == init_task0 || task->state != TASK_INTERRUPTIBLE) {
        mm_irq_restore(flags);
        return -1;
    }
    task->state = TASK_READY;
    list_add_tail(&task->ready_node, &ready_task_head);
    mm_irq_restore(flags);
    return 0;
}
