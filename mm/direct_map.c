#include <direct_map.h>
#include <physical_memory.h>
#include <kernel.h>

#define TABLE_SPAN (4096U * 1024)
#define DIRECT_TABLES (DIRECT_MAP_SIZE / TABLE_SPAN)
static int ready;

typedef char direct_map_alignment_check[(DIRECT_MAP_SIZE % TABLE_SPAN == 0 &&
    KERNEL_VIRT_BASE % TABLE_SPAN == 0) ? 1 : -1];

int direct_map_contains_physical(uint32_t address, uint32_t size)
{
    uint32_t limit = ready ? DIRECT_MAP_SIZE : BOOT_DIRECT_MAP_SIZE;
    return size && address < limit && size <= limit - address;
}

void *direct_map_pointer(uint32_t physical)
{
    if (!direct_map_contains_physical(physical, 1)) return 0;
    return (void *)(KERNEL_VIRT_BASE + physical);
}

int direct_map_physical(const void *pointer, uint32_t *physical)
{
    uint32_t address = (uint32_t)pointer;
    if (!physical || address < KERNEL_VIRT_BASE ||
        !direct_map_contains_physical(address - KERNEL_VIRT_BASE, 1)) return -1;
    *physical = address - KERNEL_VIRT_BASE;
    return 0;
}

int direct_map_build(page_directory_t *directory)
{
    uint32_t first = KERNEL_VIRT_BASE >> 22, i, j;
    if (!directory || ready) return -1;
    /* Reject occupied slots before making changes. */
    for (i = 0; i < DIRECT_TABLES; i++)
        if (directory->entries[first + i]) return -1;
    for (i = 0; i < DIRECT_TABLES; i++) {
        uint32_t physical = alloc_page();
        uint32_t *table;
        if (!physical) break;
        table = direct_map_pointer(physical);
        if (!table) { free_page(physical); break; }
        for (j = 0; j < 1024; j++)
            table[j] = (i * TABLE_SPAN + j * 4096) | PAGE_PRESENT | PAGE_WRITE;
        /* Supervisor only. Target RAM pages are NOT allocated here. */
        directory->entries[first + i] = physical | PAGE_PRESENT | PAGE_WRITE;
    }
    if (i == DIRECT_TABLES) return 0;
    while (i) {
        uint32_t slot = first + --i;
        free_page(directory->entries[slot] & ~4095U);
        directory->entries[slot] = 0;
    }
    return -1;
}

/* Called only after loading the newly built directory into CR3. */
void direct_map_activate(void)
{
    ready = 1;
    physical_memory_enable_direct_map();
}
int direct_map_is_ready(void) { return ready; }
