#include <printf.h>
#include <stdint.h>
#include <physical_memory.h>
#include <process.h>
#include <pid.h>
#include <heap.h>
#include <virtual_memory.h>
#include <kernel.h>
#include <common.h>

static uint32_t copy_content(void)
{
    uint32_t physical = alloc_page();
    if (!physical) return 0;
    uint32_t*new_task=(uint32_t*)(physical+PAGE_OFFSET);
    uint32_t*current_task=current;
    loop(1024)
    {
        *(new_task+i)=*(current_task+i);
    }

    return (uint32_t)new_task;

}

static uint32_t copy_page_directory_table(void)
{
    page_directory_t*p_pgd=(page_directory_t*)alloc_page();
    if (!p_pgd) return 0;
    page_directory_t*v_pgd=(page_directory_t *)((uint32_t)p_pgd+PAGE_OFFSET);
    for (uint32_t i = 0; i < PAGE_DIR_ENTRIES; i++) v_pgd->entries[i] = 0;
    ////printf("%x\n",current->mm->pgd);
    uint32_t index=PAGE_OFFSET>>22;
    for(index;index<PAGE_DIR_ENTRIES;index++)
    {
        v_pgd->entries[index]=current->mm->pgd->entries[index];
    }

    return (uint32_t)v_pgd;
}

extern void fork_return(void);

int do_fork(const uint32_t *frame)
{
    if (!current || !frame || (frame[11] & 3U) != 3U) return -1;
    task_t*new_task=(task_t*)copy_content();
    if (!new_task) return -1;
    page_directory_t*pgd= (page_directory_t*)copy_page_directory_table();

    if (!pgd) { free_page((uint32_t)new_task - PAGE_OFFSET); return -1; }
    new_task->pid=pid_alloc();
    if (new_task->pid == PID_INVALID) {
        free_page((uint32_t)pgd - PAGE_OFFSET);
        free_page((uint32_t)new_task - PAGE_OFFSET);
        return -1;
    }
    new_task->ppid=current->pid;
    new_task->state=TASK_READY;
    new_task->mm=(mm_struct_t*)kmalloc(sizeof(mm_struct_t));
    if (!new_task->mm) {
        pid_free(new_task->pid);
        free_page((uint32_t)pgd - PAGE_OFFSET);
        free_page((uint32_t)new_task - PAGE_OFFSET);
        return -1;
    }
    *new_task->mm = *current->mm;
    new_task->on_cpu=0;
    new_task->time_slice=PROCESS_TIME_SLICE;
    new_task->mm->pgd=pgd;

    /* Ten saved register slots followed by the five-word user iret frame. */
    uint32_t *child_frame = (uint32_t *)((uint32_t)new_task + PAGE_SIZE) - 15;
    for (uint32_t i = 0; i < 15; i++) child_frame[i] = frame[i];
    child_frame[0] = 0; /* fork returns zero in the child. */
    task_context_t *context = (task_context_t *)child_frame - 1;
    *context = (task_context_t){0};
    context->eip = (uint32_t)fork_return;
    new_task->context = context;
    INIT_LIST_HEAD(&new_task->ready_node);
    INIT_LIST_HEAD(&new_task->all_tasks_node);

    INIT_LIST_HEAD(&new_task->mm->vm_area_list);
    if (mm_copy(new_task->mm,current->mm)) {
        pid_free(new_task->pid);
        free_page((uint32_t)new_task - PAGE_OFFSET);
        return -1; /* mm_copy performs address-space cleanup on failure. */
    }


    list_add_tail(&new_task->all_tasks_node,&all_task_head);
    list_add_tail(&new_task->ready_node,&ready_task_head);


    return new_task->pid;
}
