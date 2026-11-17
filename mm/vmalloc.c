#include <vmalloc.h>
#include <physical_memory.h>
#include <paging.h>
#include <mm_lock.h>
#include "virtual_area.h"

static void release_pages(struct virtual_area *area, uint32_t bytes)
{
    uint32_t offset, physical;
    for (offset = 0; offset < bytes; offset += 4096) {
        if (!translate_virtual_to_physical(_pdt, area->start + offset, &physical)) {
            unmap_virtual_address(_pdt, area->start + offset);
            free_page(physical & ~4095U);
        }
    }
}

void *vmalloc(uint32_t size)
{
    uint32_t flags, offset, physical;
    struct virtual_area *area;
    if (!size || size > 0xfffff000U) return 0;
    size = (size + 4095U) & ~4095U;
    flags = mm_irq_save();
    area = virtual_area_reserve(size, VAREA_RAM);
    if (!area) { mm_irq_restore(flags); return 0; }
    for (offset = 0; offset < size; offset += 4096) {
        physical = alloc_page();
        if (!physical) break;
        if (map_virtual_to_physical(_pdt, area->start + offset, physical,
                                    PAGE_PRESENT | PAGE_WRITE)) {
            free_page(physical);
            break;
        }
    }
    if (offset != size) {
        release_pages(area, offset);
        virtual_area_release(area);
        mm_irq_restore(flags);
        return 0;
    }
    void *result = (void *)area->start;
    mm_irq_restore(flags);
    return result;
}

void *vzalloc(uint32_t size)
{
    uint8_t *pointer = vmalloc(size);
    uint32_t i;
    if (pointer) for (i = 0; i < size; i++) pointer[i] = 0;
    return pointer;
}

void vfree(void *pointer)
{
    uint32_t flags;
    struct virtual_area *area;
    if (!pointer) return;
    flags = mm_irq_save();
    area = virtual_area_find((uint32_t)pointer, VAREA_RAM);
    if (area) { release_pages(area, area->size); virtual_area_release(area); }
    mm_irq_restore(flags);
}
