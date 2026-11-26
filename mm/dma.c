#include <dma.h>
#include <physical_memory.h>
#include <kernel.h>
#include <mm_lock.h>
#include <direct_map.h>

#define DMA_RECORD_COUNT 128U
static struct { void *pointer; uint32_t physical, pages; } allocations[DMA_RECORD_COUNT];

int dma_map_buffer(const void *pointer, uint32_t size, uint32_t mask, dma_addr_t *handle)
{
    uint32_t physical;
    if (!handle || !direct_map_is_ready() ||
        direct_map_physical(pointer, &physical) ||
        !direct_map_contains_physical(physical, size)) return -1;
    if ((uint64_t)physical + size - 1 > mask) return -1;
    __asm__ volatile("" ::: "memory");
    *handle = physical;
    return 0;
}

void dma_unmap_buffer(dma_addr_t handle, uint32_t size)
{
    (void)handle; (void)size;
    __asm__ volatile("" ::: "memory");
}

void *dma_alloc_coherent(uint32_t size, uint32_t mask, dma_addr_t *handle)
{
    uint32_t flags, i, pages, physical, j;
    uint8_t *pointer;
    if (!size || !handle || size > 0xfffff000U) return 0;
    flags = mm_irq_save();
    for (i = 0; i < DMA_RECORD_COUNT && allocations[i].pointer; i++);
    if (i == DMA_RECORD_COUNT) { mm_irq_restore(flags); return 0; }
    pages = (size + 4095U) >> 12;
    physical = alloc_pages(pages);
    if (!physical) { mm_irq_restore(flags); return 0; }
    pointer = (void *)(physical + KERNEL_VIRT_BASE);
    if (dma_map_buffer(pointer, pages << 12, mask, handle)) {
        free_pages(physical, pages);
        mm_irq_restore(flags);
        return 0;
    }
    allocations[i].pointer = pointer;
    allocations[i].physical = physical;
    allocations[i].pages = pages;
    for (j = 0; j < (pages << 12); j++) pointer[j] = 0;
    mm_irq_restore(flags);
    return pointer;
}

void dma_free_coherent(void *pointer)
{
    uint32_t flags = mm_irq_save(), i;
    for (i = 0; i < DMA_RECORD_COUNT; i++)
        if (pointer && allocations[i].pointer == pointer) {
            free_pages(allocations[i].physical, allocations[i].pages);
            allocations[i].pointer = 0;
            break;
        }
    mm_irq_restore(flags);
}
