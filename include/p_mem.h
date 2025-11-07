#ifndef MEMORY_H
#define MEMORY_H

#define PAGE_ALIGN_UP(addr)                        ((addr+0xFFF)&(~0xFFF))
#define PAGE_ALIGN_DOWN(addr)                   (addr&(~0xFFF))

unsigned long alloc_page();
void free_page(unsigned long addr);

#endif