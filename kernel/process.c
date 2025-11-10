#include <process.h>
#include <stdint.h>
#include <p_mem.h>

task_t init_task={
    .pid=0,
    .ppid=-1,
    .pdt=&pdt,
    .state=TASK_RUNNING,
    .on_cpu=0,

};

struct tss_struct  tss_globel __attribute__((aligned(4)));


void _init_task0()
{
    uint32_t addr =(uint32_t)alloc_page();
    tss_globel.esp0=addr+0x1000;
    tss_globel.ss0=0x10;
    current=&init_task;
}