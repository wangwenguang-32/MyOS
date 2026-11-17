#include <phy_mem.h>
#include <kmalloc.h>
#include <stdint.h>
#include <iomap.h>
#include <addr_translation.h>

void init_memory(void)
{
    _init_phy_mem();
    _init_paging();
    _init_kmalloc();
    _init_iomap();
}
