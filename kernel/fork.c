#include<printf.h>
#include<stdint.h>
#include<p_mem.h>
#include<process.h>
#include<pid.h>

void copy_content(uint32_t addr)
{
    uint32_t*new_task=(uint32_t*)addr;
    uint32_t*current_task=current;
    uint32_t i=0;
    for(i;i<1024;i++)
    {
        *(new_task+i)=*(current_task+i);
    }

}

int sys_fork()
{
    uint32_t addr=alloc_page();
    copy_content(addr);
    task_t*new_task=(task_t*)addr;
    


    return 1;
}