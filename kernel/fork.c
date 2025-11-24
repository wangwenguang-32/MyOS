#include<printf.h>
#include<stdint.h>
#include<phy_mem.h>
#include<process.h>
#include<pid.h>
#include<kmalloc.h>
#include<virt_mem.h>
#include<kernel.h>

uint32_t copy_content()
{
    uint32_t*new_task=(uint32_t*)(alloc_page()+PAGE_OFFSET);
    uint32_t*current_task=current;
    uint32_t i=0;
    for(i;i<1024;i++)
    {
        *(new_task+i)=*(current_task+i);
    }

    return new_task;

}

uint32_t copy_page_directory_table()
{
    page_directory_t*p_pgd=(page_directory_t*)alloc_page();
    page_directory_t*v_pgd=(uint32_t)p_pgd+PAGE_OFFSET;
    uint32_t i=0;
    for(i;i<1024;i++)
    {
        v_pgd->entries[i]=current->mm->pgd->entries[i];
    }

    return v_pgd;
}

int do_fork(uint32_t esp0)
{
    task_t*new_task=(task_t*)copy_content();
    page_directory_t*pgd= (page_directory_t*)copy_page_directory_table();

    new_task->pid=pid_alloc();
    new_task->ppid=current->pid;
    new_task->state=TASK_READY;
    new_task->mm=(mm_struct_t*)kmalloc(sizeof(mm_struct_t));
    new_task->on_cpu=0;
    new_task->ks.esp0=(uint32_t)new_task+0x1000u-((uint32_t)current+0x1000u-esp0);
    new_task->ks.ss0=0x10;
    new_task->time_slice=PROCESS_TIME_SLICE;
    new_task->mm->pgd=pgd;

    INIT_LIST_HEAD(&new_task->mm->vm_area_list);
    mm_copy(new_task->mm,current->mm);



    list_add_tail(&new_task->all_tasks_node,&all_task_head);
    list_add_tail(&new_task->ready_node,&ready_task_head);


    return new_task->pid;
}