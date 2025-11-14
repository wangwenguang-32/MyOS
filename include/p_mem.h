#ifndef  P_MEM_H
#define P_MEM_H

#include<stdint.h>

#define PAGE_ALIGN_DOWN(addr) (addr & ~0xFFF)
#define PAGE_ALIGN_UP(addr)   ((addr + 0xFFF) & ~0xFFF)

uint32_t alloc_page();
void free_page(uint32_t addr);

#endif