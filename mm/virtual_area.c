#include "virtual_area.h"
#include <kernel.h>
#include <paging.h>
#include <physical_memory.h>

#define MAX_VIRTUAL_AREAS 256U
static struct virtual_area records[MAX_VIRTUAL_AREAS];
static struct virtual_area *areas;
static int virtual_areas_ready;

struct virtual_area *virtual_area_reserve(uint32_t size, uint32_t kind)
{
    uint32_t i, start = kind == VAREA_RAM ? VMALLOC_START : IOREMAP_START;
    uint32_t limit = kind == VAREA_RAM ? VMALLOC_END + 1 : IOREMAP_END + 1;
    struct virtual_area **link = &areas, *record = 0;
    if (!virtual_areas_ready || (kind != VAREA_RAM && kind != VAREA_IO)) return 0;
    if (!size || size > limit - start || (size & 4095U)) return 0;
    for (i = 0; i < MAX_VIRTUAL_AREAS; i++)
        if (!records[i].size) { record = &records[i]; break; }
    if (!record) return 0;
    while (*link) {
        struct virtual_area *next = *link;
        if (next->start >= limit) break;
        if (next->start >= start && size <= next->start - start) break;
        if (next->start + next->size > start) start = next->start + next->size;
        link = &next->next;
    }
    if (start > limit || size > limit - start) return 0;
    record->start = start;
    record->size = size;
    record->offset = 0;
    record->kind = kind;
    record->next = *link;
    *link = record;
    return record;
}

struct virtual_area *virtual_area_find(uint32_t address, uint32_t kind)
{
    struct virtual_area *area;
    for (area = areas; area; area = area->next)
        if (area->kind == kind && area->start + area->offset == address) return area;
    return 0;
}

void virtual_area_release(struct virtual_area *area)
{
    struct virtual_area **link = &areas;
    while (*link && *link != area) link = &(*link)->next;
    if (*link) { *link = area->next; area->size = 0; area->next = 0; }
}

/* Materialize kernel PDEs before tasks copy the kernel half of the directory.
 * Page tables remain shared; future mappings update existing shared tables. */
int virtual_area_init(void)
{
    uint32_t index, physical, start = VMALLOC_START >> 22;
    uint32_t end = (IOREMAP_END >> 22) + 1;
    uint32_t created[64], count = 0;
    if (virtual_areas_ready) return 0;
    if (!_pdt || end - start > 64) return -1;
    for (index = start; index < end; index++) {
        if (_pdt->entries[index] & PAGE_PRESENT) continue;
        physical = alloc_page();
        if (!physical) {
            while (count) {
                uint32_t slot = created[--count];
                free_page(_pdt->entries[slot] & ~4095U);
                _pdt->entries[slot] = 0;
            }
            return -1;
        }
        uint32_t *table = (void *)(physical + KERNEL_VIRT_BASE);
        for (uint32_t i = 0; i < 1024; i++) table[i] = 0;
        _pdt->entries[index] = physical | PAGE_PRESENT | PAGE_WRITE;
        created[count++] = index;
    }
    virtual_areas_ready = 1;
    return 0;
}
