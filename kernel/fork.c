#include<printf.h>
#include<stdint.h>
#include<p_mem.h>
#include<process.h>
#include<pid.h>

uint32_t copy_content()
{
    uint32_t*new_task=(uint32_t*)(alloc_page()+0xC0000000);
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
    page_directory_t*pdt=(page_directory_t*)alloc_page();
    uint32_t i=0;
    for(i;i<1024;i++)
    {
        pdt->entries[i]=current->pdt->entries[i];
    }

    return pdt;
}

int do_fork(uint32_t esp0)
{
    task_t*new_task=(task_t*)copy_content();
    page_directory_t*pdt= (page_directory_t*)copy_page_directory_table();

    new_task->pid=pid_alloc();
    //new_task->pid=100;
    new_task->ppid=current->pid;
    new_task->state=TASK_READY;
    new_task->pdt=pdt;
    new_task->on_cpu=0;
    new_task->ks.esp0=(uint32_t)new_task+0x1000u-((uint32_t)current+0x1000u-esp0);
    new_task->ks.ss0=0x10;
    new_task->time_slice=PROCESS_TIME_SLICE;



    list_add_tail(&new_task->all_tasks_node,&all_task_head);
    list_add_tail(&new_task->ready_node,&ready_task_head);


    return new_task->pid;
}