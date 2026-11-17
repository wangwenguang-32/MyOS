#include <iomap.h>
#include <stdint.h>
#include <kernel.h>
#include <kmalloc.h>
#include <addr_translation.h>

vm_iomap_area_t*iomap_free_list=NULL;

void vm_iomap_add_list(vm_iomap_area_t*area)
{
    area->next = NULL;
    if(!iomap_free_list)
    {
        iomap_free_list=area;
        return;
    }

    vm_iomap_area_t**p = &iomap_free_list;
    while(*p)
    {
        if(area->size<=((*p)->size))
        {
            area->next = *p;
            *p = area;
            return;
        }
        p = &(*p)->next;

    }
    *p = area;
}

vm_iomap_area_t* find_free(uint32_t size)
{
    vm_iomap_area_t**p = &iomap_free_list;
    while(*p)
    {
        if(size<=(*p)->size)
        {
            vm_iomap_area_t*tmp = *p;
            *p = (*p)->next;
            return tmp;
        }
        p = &(*p)->next;
    }
    return NULL;
}

void merge_free_area(vm_iomap_area_t*area)
{
    vm_iomap_area_t** p = &iomap_free_list;
    while(*p)
    {
        if(area->vm_start + (area)->size == (*p)->vm_start)
        {
            vm_iomap_area_t* free_area= *p;
            *p = (*p)->next;
            area->size = area->size + free_area->size;
            kfree(free_area);
            p = &iomap_free_list;
            continue;
        }

        if((uint32_t)((*p)->vm_start) + (*p)->size == area->vm_start)
        {
            vm_iomap_area_t* free_area= *p;
            *p = (*p)->next;
            area->size = area->size + free_area->size ;
            area->vm_start = free_area->vm_start;
            kfree(free_area);
            p = &iomap_free_list;
            continue;
        }

        p = &(*p)->next;
    }
}


uint32_t iomap(uint32_t phy_addr, uint32_t size)
{
    uint32_t offset = phy_addr & 4095U;
    uint64_t length = ((uint64_t)size + offset + 4095) & ~4095ULL;
    vm_iomap_area_t *area;
    uint32_t address;
    if (!size || length > IOREMAP_SIZE ||
        (uint64_t)phy_addr + size > (1ULL << 32)) return 0;
    area = find_free((uint32_t)length);
    if (!area) return 0;
    address = area->vm_start;
    /* PCD and PWT prevent normal RAM caching of device registers. */
    if (map_virtual_range_to_physical(_pdt, address, phy_addr & ~4095U,
                                     length >> 12, PAGE_PRESENT | PAGE_WRITE | 0x18)) {
        unmap_virtual_address_range(_pdt, address, length >> 12);
        vm_iomap_add_list(area);
        return 0;
    }
    area->size -= length;
    if (area->size) {
        area->vm_start += length;
        vm_iomap_add_list(area);
    } else kfree(area);
    return address + offset;
}

void free_iomap_area(vm_iomap_area_t* area, uint32_t size)
{
    if (!area)
    {
        return;
    }
    area->size = size;

    unmap_virtual_address_range(_pdt,(uint32_t)(area->vm_start),size>>12);

    merge_free_area(area);

    vm_iomap_add_list(area);
}

void _init_iomap()
{
    vm_iomap_area_t*area = (vm_iomap_area_t*)kmalloc(sizeof(vm_iomap_area_t));
    if (!area || iomap_free_list) { if (area) kfree(area); return; }
    area->vm_start = IOREMAP_START;
    area->size = IOREMAP_SIZE;
    vm_iomap_add_list(area);
}
