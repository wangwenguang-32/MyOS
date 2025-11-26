#include<process.h>
#include<stdint.h>
#include<phy_mem.h>
#include<section.h>
#include<addr_translation.h>
#include<kernel.h>
#include<virt_mem.h>
#include<gdt.h>

extern struct tss_struct  tss_globel;

task_t* pick_next_task(void)
{
    if (list_empty(&ready_task_head)) {
        return current; 
    }
    
    struct list_head *node = ready_task_head.next;
    task_t*next;
    next=list_entry(node,task_t,ready_node);
    return next;
}
/* 主调度函数 */
uint32_t schedule(uint32_t prev_esp0)
{
    task_t* prev = current;
    task_t* next = 0;
    
    if (prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
        prev->time_slice=PROCESS_TIME_SLICE;
        prev->ks.esp0=prev_esp0;
        prev->ks.ss0=0x10;
        list_add_tail(&prev->ready_node,&ready_task_head);
    }

    
    next = pick_next_task();
    
        if (next->state == TASK_READY) {
            list_del(&next->ready_node);
        }
        
        next->state = TASK_RUNNING;
        next->on_cpu = 0;
        current = next;
        tss_globel.esp0= (uint32_t) next + 0x1000u;
        tss_globel.ss0=next->ks.ss0;
        uint32_t pgd_phys=(uint32_t)current->mm->pgd-PAGE_OFFSET;
        __asm__ __volatile__("movl %%eax,%%cr3;"::"a"(pgd_phys));
        return next->ks.esp0;
}
