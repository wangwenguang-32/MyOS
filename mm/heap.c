#include <kmalloc.h>
#include <phy_mem.h>
#include <kernel.h>
#include <stdint.h>

#define HEAP_ALIGNMENT 8U
#define PAGE_BYTES 4096U

typedef struct heap_block {
    uint32_t size;
    struct heap_block *next;
} heap_block_t;

static heap_block_t *free_list;

/* Keep free blocks in address order, coalescing both neighbors. */
static void insert_free_block(heap_block_t *block)
{
    heap_block_t **link = &free_list;
    heap_block_t *previous = 0;
    while (*link && (uintptr_t)*link < (uintptr_t)block) {
        previous = *link;
        link = &(*link)->next;
    }
    block->next = *link;
    *link = block;
    if (block->next && (uint8_t *)(block + 1) + block->size == (uint8_t *)block->next) {
        block->size += sizeof(*block) + block->next->size;
        block->next = block->next->next;
    }
    if (previous && (uint8_t *)(previous + 1) + previous->size == (uint8_t *)block) {
        previous->size += sizeof(*block) + block->size;
        previous->next = block->next;
    }
}

void _init_kmalloc(void) { /* Pages are acquired lazily after paging initialization. */ }

uint32_t kmalloc(uint32_t size)
{
    heap_block_t **link, *block;
    uint32_t pages, physical;
    if (!size || size > 0xffffffffU - sizeof(*block) - PAGE_BYTES) return 0;
    size = (size + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);
    for (link = &free_list; *link; link = &(*link)->next)
        if ((*link)->size >= size) break;
    if (*link) {
        block = *link;
        *link = block->next;
    } else {
        pages = (size + sizeof(*block) + PAGE_BYTES - 1) / PAGE_BYTES;
        physical = alloc_pages(pages);
        if (!physical) return 0;
        block = (heap_block_t *)(physical + KERNEL_VIRT_BASE);
        block->size = pages * PAGE_BYTES - sizeof(*block);
    }
    if (block->size >= size + sizeof(*block) + HEAP_ALIGNMENT) {
        heap_block_t *remainder = (void *)((uint8_t *)(block + 1) + size);
        remainder->size = block->size - size - sizeof(*block);
        insert_free_block(remainder);
        block->size = size;
    }
    block->next = 0;
    return (uint32_t)(block + 1);
}

void kfree(void *pointer)
{
    if (pointer) insert_free_block((heap_block_t *)pointer - 1);
}
