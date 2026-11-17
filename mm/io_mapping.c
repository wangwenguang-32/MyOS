#include <io_mapping.h>
#include <paging.h>
#include <mm_lock.h>
#include "virtual_area.h"

uint32_t iomap(uint32_t physical, uint32_t size)
{
    uint32_t flags, offset, mapped = 0;
    uint64_t length = ((uint64_t)size + (physical & 4095U) + 4095U) & ~4095ULL;
    struct virtual_area *area;
    if (!size || length > 0xfffff000ULL ||
        (uint64_t)physical + size > (1ULL << 32)) return 0;
    flags = mm_irq_save();
    area = virtual_area_reserve((uint32_t)length, VAREA_IO);
    if (!area) { mm_irq_restore(flags); return 0; }
    area->offset = physical & 4095U;
    for (offset = 0; offset < length; offset += 4096) {
        if (map_virtual_to_physical(_pdt, area->start + offset,
            (physical & ~4095U) + offset, PAGE_PRESENT | PAGE_WRITE | 0x18U)) break;
        mapped += 4096;
    }
    if (mapped != length) {
        for (offset = 0; offset < mapped; offset += 4096)
            unmap_virtual_address(_pdt, area->start + offset);
        virtual_area_release(area);
        mm_irq_restore(flags);
        return 0;
    }
    uint32_t result = area->start + area->offset;
    mm_irq_restore(flags);
    return result;
}

void iounmap(uint32_t address)
{
    uint32_t flags = mm_irq_save(), offset;
    struct virtual_area *area = virtual_area_find(address, VAREA_IO);
    if (area) {
        for (offset = 0; offset < area->size; offset += 4096)
            unmap_virtual_address(_pdt, area->start + offset);
        virtual_area_release(area);
    }
    mm_irq_restore(flags);
}

void free_iomap_area(vm_iomap_area_t *area, uint32_t size)
{
    (void)size;
    if (area) iounmap(area->vm_start);
}

int _init_iomap(void) { return virtual_area_init(); }
