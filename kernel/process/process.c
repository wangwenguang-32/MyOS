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
extern void init_main(void *argument);
extern int prepare_user_init(task_t *task);

static void idle_main(void *argument)
{
    (void)argument;
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
    task_entry_t entry = current->entry;
    void *argument = current->entry_argument;
    __asm__ volatile("sti" ::: "memory");
    entry(argument);
    /* A returned task is retained as a zombie; its live stack must not be freed. */
    for (;;) {
        __asm__ volatile("cli" ::: "memory");
        current->exit_code = 0;
        current->state = TASK_ZOMBIE;
        schedule();
    }
}

typedef char task_stack_room_check[(sizeof(task_t) + sizeof(task_context_t) + 1024 < PAGE_SIZE) ? 1 : -1];

static task_t *allocate_task(int32_t pid, sched_policy_t policy,
                             const char *name, task_entry_t entry, void *argument)
{
    uint32_t physical = alloc_page();
    task_t *task;
    task_context_t *context;
    if (!physical) return 0;
    task = (void *)(physical + PAGE_OFFSET);
    for (uint32_t i = 0; i < PAGE_SIZE; i++) ((uint8_t *)task)[i] = 0;
    task->pid = pid;
    task->ppid = pid ? 0 : -1;
    task->state = TASK_NEW;
    task->entry = entry;
    task->entry_argument = argument;
    task->flags = TASK_FLAG_KERNEL_THREAD;
    for (uint32_t i = 0; name && name[i] && i + 1 < TASK_NAME_LEN; i++)
        task->comm[i] = name[i];
    task->policy = policy;
    task->time_slice = PROCESS_TIME_SLICE;
    task->mm = &kernel_mm;
    INIT_LIST_HEAD(&task->ready_node);
    INIT_LIST_HEAD(&task->all_tasks_node);
    INIT_LIST_HEAD(&task->sibling_node);
    INIT_LIST_HEAD(&task->block_node);
    INIT_LIST_HEAD(&task->sleep_node);
    /* ret enters with ESP = top-4, matching i386 C function entry alignment. */
    context = (void *)((uint32_t)task + PAGE_SIZE - 4 - sizeof(*context));
    context->eip = (uint32_t)kernel_task_entry;
    task->context = context;
    return task;
}

task_t *create_kernel_task(const char *name, task_entry_t entry, void *argument)
{
    uint32_t flags;
    int32_t pid;
    task_t *task;
    if (!entry || !kernel_mm.pgd) return 0;
    flags = mm_irq_save();
    pid = pid_alloc();
    if (pid == PID_INVALID) { mm_irq_restore(flags); return 0; }
    task = allocate_task(pid, SCHED_NORMAL, name, entry, argument);
    if (!task) pid_free(pid);
    else task->ppid = current ? current->pid : 0;
    mm_irq_restore(flags);
    return task;
}

int task_start(task_t *task)
{
    uint32_t flags = mm_irq_save();
    if (!task || task == init_task0 || task->state != TASK_NEW ||
        !task->context || !task->entry || !task->mm || !task->mm->pgd) {
        mm_irq_restore(flags);
        return -1;
    }
    list_add_tail(&task->all_tasks_node, &all_task_head);
    task->state = TASK_READY;
    list_add_tail(&task->ready_node, &ready_task_head);
    mm_irq_restore(flags);
    return 0;
}

int task_discard(task_t *task)
{
    uint32_t flags = mm_irq_save();
    if (!task || task == init_task0 || task->state != TASK_NEW || task->mm != &kernel_mm) {
        mm_irq_restore(flags);
        return -1;
    }
    pid_free(task->pid);
    free_page((uint32_t)task - PAGE_OFFSET);
    mm_irq_restore(flags);
    return 0;
}

int init_task(void)
{
    int32_t pid;
    if (current) return 0;
    kernel_mm = (mm_struct_t){0};
    kernel_mm.pgd = _pdt;
    INIT_LIST_HEAD(&kernel_mm.vm_area_list);
    pid_allocator_init();
    init_task0 = allocate_task(0, SCHED_IDLE, "idle", idle_main, 0);
    if (!init_task0) return -1;
    pid = pid_alloc();
    if (pid != 1) { free_page((uint32_t)init_task0 - PAGE_OFFSET); init_task0 = 0; return -1; }
    init_process = allocate_task(pid, SCHED_NORMAL, "init", init_main, 0);
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
    init_task0->state = TASK_READY;
    init_process->flags &= ~TASK_FLAG_KERNEL_THREAD;
    if (task_start(init_process)) return -1;
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
