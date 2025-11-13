#include <process.h>
#include <stdint.h>
#include <p_mem.h>


LIST_HEAD(ready_task_head);
LIST_HEAD(all_task_head);
LIST_HEAD(wait_task_head);

extern uint32_t  task0_phys_start;

#define sym_val(x)             ((uint32_t)&x)
#define pa(x)                     (x-0xC0000000)


task_t init_task={
    .pid=0,
    .ppid=-1,
    .pdt=&pdt,
    .state=TASK_RUNNING,
    .on_cpu=0,

};

struct tss_struct  tss_globel __attribute__((aligned(4)));

task_t* init_task0 __attribute__((section(".data"))) ;


void _init_task0()
{
    uint32_t addr =(uint32_t)alloc_page();
    tss_globel.esp0=addr+0x1000;
    tss_globel.ss0=0x10;

    init_task0=(task_t*)addr;

    uint32_t task0_start=  sym_val(task0_phys_start);
    printf("%x\n",task0_start);
    map_virtual_to_physical(0x8048000u,task0_start,0x07);
    current=&init_task;
}