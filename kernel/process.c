#include <process.h>
#include <stdint.h>
#include <p_mem.h>
#include <section.h>
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
    init_task0->pdt=pdt;
    init_task0->on_cpu=0;
}


void _init_task0()
{
    uint32_t addr =(uint32_t)alloc_page();
    uint32_t stack_addr=(uint32_t)alloc_page();
    tss_globel.esp0=addr+0x1000;
    tss_globel.ss0=0x10;

    init_task0=(task_t*)addr;
    init_task_t();

    uint32_t task0_start=  sym_val(task0_phys_start);
    map_virtual_to_physical(0x8048000u,task0_start,0x07u);
    map_virtual_to_physical(0xC0000000-0x1000,stack_addr,0x07u);
    current=init_task0;
}