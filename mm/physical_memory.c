#include <physical_memory.h>
#include <kernel.h>
#include <multiboot2.h>
#include <sections.h>
#include <stdint.h>
#include <mm_lock.h>
#include <direct_map.h>

#define PHYSICAL_PAGE_COUNT (1U << 20)
#define PAGE_USED 1U
#define PAGE_FREE 0U
#define PAGE_SHIFT 12U
#define PAGE_BYTES (1U << PAGE_SHIFT)
#define BOOT_RESERVATION_LIMIT 32U

/* One bit per 4 KiB physical page in the 32-bit address space. */
static uint8_t page_bitmap[PHYSICAL_PAGE_COUNT / 8] DATA;
/* Only pages handed out by this allocator may be released. */
static uint8_t allocated_bitmap[PHYSICAL_PAGE_COUNT / 8];
static uint32_t allocation_failures, invalid_frees;
static uint32_t free_page_count, allocated_page_count;
static uint32_t next_page = 1;
static uint32_t allocation_limit = BOOT_DIRECT_MAP_SIZE >> 12;
static int physical_ready;
struct memory_region mem_regions[MAX_MAP_NUM];
int num_region;
static struct { uint32_t start, end; } boot_reservations[BOOT_RESERVATION_LIMIT];
static uint32_t reservation_count;
static int boot_map_invalid;

void reserve_boot_memory(uint32_t start, uint32_t end)
{
    if (physical_ready) return;
    if (end <= start || reservation_count == BOOT_RESERVATION_LIMIT) { boot_map_invalid = 1; return; }
    boot_reservations[reservation_count].start = start;
    boot_reservations[reservation_count++].end = end;
}


void setup_mmap(struct multiboot_tag_mmap *tag)
{
    uint32_t offset;
    if (physical_ready) return;
    num_region = 0;
    if (!tag || tag->size < sizeof(*tag) ||
        tag->entry_size < sizeof(multiboot_memory_map_t) ||
        tag->entry_size > tag->size - sizeof(*tag)) {
        boot_map_invalid = 1;
        return;
    }
    for (offset = sizeof(*tag); offset <= tag->size - tag->entry_size;
         offset += tag->entry_size) {
        multiboot_memory_map_t *entry = (void *)((uint8_t *)tag + offset);
        uint64_t end;
        if (num_region == MAX_MAP_NUM) { boot_map_invalid = 1; break; }
        if (entry->addr >= (1ULL << 32) || !entry->len) continue;
        end = entry->len > (1ULL << 32) - entry->addr ?
              (1ULL << 32) : entry->addr + entry->len;
        mem_regions[num_region].base_addr = entry->addr;
        mem_regions[num_region].length = end - entry->addr;
        mem_regions[num_region++].type = entry->type;
    }
}

static int bitmap_test(const uint8_t *bitmap, uint32_t page)
{
    return (bitmap[page >> 3] >> (page & 7U)) & 1U;
}

/* Set an arbitrary page range, processing complete bytes in one operation. */
static void bitmap_update(uint8_t *bitmap, uint32_t first, uint32_t count, int used)
{
    while (count) {
        uint32_t offset = first & 7U;
        uint32_t bits = 8U - offset;
        if (bits > count) bits = count;
        uint8_t mask = (uint8_t)(((1U << bits) - 1U) << offset);
        if (used) bitmap[first >> 3] |= mask;
        else bitmap[first >> 3] &= (uint8_t)~mask;
        first += bits;
        count -= bits;
    }
}

/* End is exclusive; zero represents the end of the 4 GiB space. */
static void set_bits(uint32_t start, uint32_t end, uint8_t used)
{
    uint32_t first = start >> PAGE_SHIFT;
    uint32_t limit = end ? end >> PAGE_SHIFT : PHYSICAL_PAGE_COUNT;
    if (first < limit) bitmap_update(page_bitmap, first, limit - first, used);
}

static uint32_t count_free_pages(uint32_t first, uint32_t limit)
{
    uint32_t count = 0;
    for (; first < limit; first++) count += !bitmap_test(page_bitmap, first);
    return count;
}

/* Scan without wrapping a contiguous run. Skip fully occupied bytes. */
static uint32_t find_run(uint32_t begin, uint32_t end, uint32_t count)
{
    uint32_t page = begin, run = 0, start = 0;
    while (page < end) {
        if (!(page & 7U) && page_bitmap[page >> 3] == 0xff) {
            page += 8;
            run = 0;
            continue;
        }
        if (!(page_bitmap[page >> 3] & (1U << (page & 7)))) {
            if (!run) start = page;
            if (++run == count) return start;
        } else run = 0;
        page++;
    }
    return 0;
}

static uint32_t allocate_pages(uint32_t count)
{
    uint32_t start;
    if (!physical_ready || !count) return 0;
    if (count > free_page_count) { allocation_failures++; return 0; }
    start = find_run(next_page, allocation_limit, count);
    /* Full rescan also finds a run crossing the search cursor. */
    if (!start && next_page != 1) start = find_run(1, allocation_limit, count);
    if (!start) { allocation_failures++; return 0; }
    set_bits(start << 12, (start + count) << 12, PAGE_USED);
    bitmap_update(allocated_bitmap, start, count, PAGE_USED);
    free_page_count -= count;
    allocated_page_count += count;
    next_page = start + count;
    if (next_page >= allocation_limit) next_page = 1;
    return start << 12;
}

uint32_t alloc_pages(uint32_t count)
{
    uint32_t flags = mm_irq_save();
    uint32_t address = allocate_pages(count);
    mm_irq_restore(flags);
    return address;
}
uint32_t alloc_page(void) { return alloc_pages(1); }

int free_pages_checked(uint32_t address, uint32_t count)
{
    uint32_t flags = mm_irq_save(), page, first = address >> 12;
    if (!physical_ready || !address || (address & 4095U) || !count ||
        first >= allocation_limit || count > allocation_limit - first) goto invalid;
    /* Validate the entire range before altering any bit. Reserved pages and
     * double frees are rejected; subranges of live allocations are allowed. */
    for (page = first; page < first + count; page++)
        if (!(allocated_bitmap[page >> 3] & (1U << (page & 7)))) goto invalid;
    bitmap_update(allocated_bitmap, first, count, PAGE_FREE);
    free_page_count += count;
    allocated_page_count -= count;
    set_bits(address, address + (count << 12), PAGE_FREE);
    if (first < next_page) next_page = first;
    mm_irq_restore(flags);
    return 0;
invalid:
    invalid_frees++;
    mm_irq_restore(flags);
    return -1;
}

void free_pages(uint32_t address, uint32_t count)
{
    (void)free_pages_checked(address, count);
}

void physical_memory_get_stats(physical_memory_stats_t *stats)
{
    uint32_t flags;
    if (!stats) return;
    flags = mm_irq_save();
    *stats = (physical_memory_stats_t){0};
    stats->allocated_pages = allocated_page_count;
    stats->free_pages = free_page_count;
    stats->managed_pages = allocated_page_count + free_page_count;
    stats->allocation_failures = allocation_failures;
    stats->invalid_frees = invalid_frees;
    mm_irq_restore(flags);
}

void free_page(uint32_t address) { free_pages(address, 1); }

static void init_mem_regions(void)
{
    uint32_t i, pass;
    for (i = 0; i < sizeof(page_bitmap); i++) page_bitmap[i] = 0xff;
    /* Free only fully usable pages, then let reserved entries take precedence. */
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < (uint32_t)num_region; i++) {
            uint64_t start = mem_regions[i].base_addr;
            uint64_t end = start + mem_regions[i].length;
            int usable = mem_regions[i].type == AddressRangeMemory;
            if (usable != (pass == 0)) continue;
            start = usable ? (start + 4095) & ~4095ULL : start & ~4095ULL;
            end = usable ? end & ~4095ULL : (end + 4095) & ~4095ULL;
            if (start < end) set_bits((uint32_t)start, (uint32_t)end, !usable);
        }
    }
    set_bits(0, 0x100000, PAGE_USED);
    set_bits(PAGE_ALIGN_DOWN(boot_kernel_start), PAGE_ALIGN_UP(boot_kernel_end), PAGE_USED);
    set_bits(PAGE_ALIGN_DOWN(pa(kernel_virt_start)), PAGE_ALIGN_UP(pa(kernel_virt_end)), PAGE_USED);
}

int _init_phy_mem(void)
{
    uint32_t i;
    if (physical_ready) return 0;
    if (boot_map_invalid || !num_region || boot_kernel_end <= boot_kernel_start ||
        kernel_virt_end <= kernel_virt_start ||
        pa(kernel_virt_end) > BOOT_DIRECT_MAP_SIZE) return -1;
    init_mem_regions();
    set_bits(PAGE_ALIGN_DOWN(task0_phys_start), PAGE_ALIGN_UP(task0_phys_end), PAGE_USED);
    for (i = 0; i < reservation_count; i++)
        set_bits(PAGE_ALIGN_DOWN(boot_reservations[i].start),
                 PAGE_ALIGN_UP(boot_reservations[i].end), PAGE_USED);
    for (i = 256; i < allocation_limit; i++)
        if (!(page_bitmap[i >> 3] & (1U << (i & 7)))) break;
    if (i == allocation_limit) return -1;
    next_page = i;
    free_page_count = count_free_pages(1, allocation_limit);
    physical_ready = 1;
    return 0;
}

void physical_memory_enable_direct_map(void)
{
    uint32_t flags = mm_irq_save();
    uint32_t limit = DIRECT_MAP_SIZE >> PAGE_SHIFT;
    if (physical_ready && allocation_limit < limit) {
        free_page_count += count_free_pages(allocation_limit, limit);
        allocation_limit = limit;
    }
    mm_irq_restore(flags);
}
