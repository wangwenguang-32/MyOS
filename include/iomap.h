#ifndef IOMAP_H
#define IOMAP_H

#include<stdint.h>



struct iomap_area
{
    uint32_t vm_start;
    uint32_t size;
    struct iomap_area*next;
    
};
typedef struct iomap_area vm_iomap_area_t;

void _init_iomap();
uint32_t iomap(uint32_t phy_addr,uint32_t size);
void free_iomap_area(vm_iomap_area_t* area, uint32_t size);


#endif