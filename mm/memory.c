#include<phy_mem.h>
#include<kmalloc.h>
#include<stdint.h>

void init_memory()
{
    _init_phy_mem();
    _init_paging();
    _init_kmalloc();
}