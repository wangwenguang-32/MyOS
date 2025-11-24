#include<process.h>
#include<stdint.h>
#include<phy_mem.h>
#include<section.h>
#include<addr_translation.h>
#include<kernel.h>
#include<virt_mem.h>
#include<gdt.h>

LIST_HEAD(ready_task_head);
LIST_HEAD(all_task_head);
LIST_HEAD(wait_task_head);

#define TASK0_VIRT_START   0x8048000u

struct tss_struct  tss_globel __attribute__((aligned(4)));

task_t* init_task0  DATA;


 void init_mm_struct_t()
 {
    mm_struct_t*mm=init_task0->mm;
    INIT_LIST_HEAD(&mm->vm_area_list);

    mm->pgd=_pdt;
    vm_area_t*code_area=alloc_vma();
    code_area->vm_start=TASK0_VIRT_START;
    code_area->vm_end=TASK0_VIRT_START+PAGE_SIZE;
    code_area->vm_type=VMA_TYPE_CODE;
    code_area->vm_flags=VM_READ|VM_EXEC|VM_PRIVATE;
    insert_vma(mm,code_area);

    vm_area_t*stack_area=alloc_vma();
    stack_area->vm_start=USER_STACK_START-PAGE_SIZE;
    stack_area->vm_end=USER_STACK_START;
    stack_area->vm_type=VMA_TYPE_STACK;
    stack_area->vm_flags=VM_READ|VM_WRITE|VM_PRIVATE|VM_GROWSDOWN;
    insert_vma(mm,stack_area);


 }



void init_task_t()
{
    init_task0->pid=0;
    init_task0->ppid= -1;
    init_task0->state= TASK_RUNNING;
    init_task0->mm=(mm_struct_t*)kmalloc(sizeof(mm_struct_t));
    init_task0->on_cpu=0;
    init_task0->policy = SCHED_NORMAL;
    init_task0->time_slice = PROCESS_TIME_SLICE;
    init_mm_struct_t();
}


void init_task()
{
    uint32_t addr =(uint32_t)alloc_page()+PAGE_OFFSET;
    uint32_t stack_addr=(uint32_t)alloc_page();
    tss_globel.esp0=addr+PAGE_SIZE;
    tss_globel.ss0=SEG_KERNEL_DATA;

    init_task0=(task_t*)addr;
    init_task_t();
    current=init_task0;

    vm_map_page(current->mm, TASK0_VIRT_START,task0_phys_start,PAGE_PRESENT|PAGE_WRITE|PAGE_USER);
    vm_map_page(current->mm, PAGE_OFFSET-PAGE_SIZE,stack_addr,PAGE_PRESENT|PAGE_WRITE|PAGE_USER);
    
    list_add(&init_task0->all_tasks_node,&all_task_head);
}



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
