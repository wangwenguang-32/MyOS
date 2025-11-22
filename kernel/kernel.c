#include<stdint.h>
#include<kernel.h>

extern uint32_t _boot_start;
extern uint32_t _boot_end;
extern uint32_t _task0_phys_start;
extern uint32_t _task0_phys_end;
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

uint32_t boot_kernel_start = sym_val(_boot_start);
uint32_t boot_kernel_end = sym_val(_boot_end);
uint32_t task0_phys_start = sym_val(_task0_phys_start);
uint32_t task0_phys_end = sym_val(_task0_phys_end);
uint32_t kernel_virt_start = sym_val(_kernel_start);
uint32_t kernel_virt_end = sym_val(_kernel_end);