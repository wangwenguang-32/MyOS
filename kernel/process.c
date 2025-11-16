#include<process.h>
#include<stdint.h>
#include<p_mem.h>
#include<section.h>
#include<page.h>

LIST_HEAD(ready_task_head);
LIST_HEAD(all_task_head);
LIST_HEAD(wait_task_head);

extern uint32_t  task0_phys_start;

#define sym_val(x)             ((uint32_t)&x)
#define pa(x)                     (x-0xC0000000)

struct tss_struct  tss_globel __attribute__((aligned(4)));

task_t* init_task0  DATA;


void init_task_t()
{
    init_task0->pid=0;
    init_task0->ppid= -1;
    init_task0->state= TASK_RUNNING;
    init_task0->pdt=_pdt;
    init_task0->on_cpu=0;
    init_task0->policy = SCHED_NORMAL;
    init_task0->time_slice = PROCESS_TIME_SLICE;
}


void _init_task0()
{
    uint32_t addr =(uint32_t)alloc_page()+0xC0000000;
    uint32_t stack_addr=(uint32_t)alloc_page();
    tss_globel.esp0=addr+0x1000;
    tss_globel.ss0=0x10;

    init_task0=(task_t*)addr;
    init_task_t();
    current=init_task0;

    uint32_t task0_start=  sym_val(task0_phys_start);
    map_virtual_to_physical(current->pdt, 0x8048000u,task0_start,0x07u);
    map_virtual_to_physical(current->pdt, 0xC0000000-0x1000,stack_addr,0x07u);
    
    list_add(&init_task0->all_tasks_node,&all_task_head);
}



/* 调度器：从运行队列中选择下一个进程 */
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
        return next->ks.esp0;
}
