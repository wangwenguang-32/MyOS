#ifndef MYOS_IO_MAPPING_H
#define MYOS_IO_MAPPING_H

#include<stdint.h>



struct iomap_area
{
    uint32_t vm_start;
    uint32_t size;
    struct iomap_area*next;
    
};
typedef struct iomap_area vm_iomap_area_t;

int _init_iomap(void);
void iounmap(uint32_t address);
static inline uint32_t readl(const volatile void *address) {
    uint32_t value = *(const volatile uint32_t *)address;
    __asm__ volatile("" ::: "memory");
    return value;
}
static inline void writel(uint32_t value, volatile void *address) {
    __asm__ volatile("" ::: "memory");
    *(volatile uint32_t *)address = value;
}

uint32_t iomap(uint32_t phy_addr,uint32_t size);
void free_iomap_area(vm_iomap_area_t* area, uint32_t size);


#endif