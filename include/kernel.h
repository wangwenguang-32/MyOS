#ifndef  KERNEL_H
#define KERNEL_H



#define sym_val(x)             ((uint32_t)&x)
#define pa(x)                     (x-0xC0000000)

extern uint32_t boot_kernel_start;
extern uint32_t boot_kernel_end;
extern uint32_t task0_phys_start;
extern uint32_t task0_phys_end;
extern uint32_t kernel_virt_start;
extern uint32_t kernel_virt_end;

#define PAGE_OFFSET     0xC0000000U
#define HIGH_MEMORY     0xF8000000U
#define DIRECT_MAP_SIZE (HIGH_MEMORY - PAGE_OFFSET)  



#endif