#include <physical_memory.h>
#include <kernel.h>
#include <multiboot2.h>
#include <sections.h>
#include <stdint.h>
#include <mm_lock.h>

#define PHYSICAL_PAGE_COUNT (1U << 20)
#define PAGE_USED 1U
#define PAGE_FREE 0U

/* One bit per 4 KiB physical page in the 32-bit address space. */
static uint8_t page_bitmap[PHYSICAL_PAGE_COUNT / 8] DATA;
static uint32_t next_page = 1;
static uint32_t allocation_limit = (8U * 1024 * 1024) >> 12;
static int physical_ready;
struct memory_region mem_regions[MAX_MAP_NUM];
int num_region;
static struct { uint32_t start, end; } boot_reservations[32];
static uint32_t reservation_count;
static int boot_map_invalid;

void reserve_boot_memory(uint32_t start, uint32_t end)
{
    if (end <= start || reservation_count == 32) { boot_map_invalid = 1; return; }
    boot_reservations[reservation_count].start = start;
    boot_reservations[reservation_count++].end = end;
}


void setup_mmap(struct multiboot_tag_mmap *tag)
{
    uint32_t offset;
    num_region = 0;
    if (!tag || tag->size < sizeof(*tag) + tag->entry_size ||
        tag->entry_size < sizeof(multiboot_memory_map_t)) return;
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

void set_bit(uint32_t address, uint8_t used)
{
    uint32_t page = address >> 12;
    uint8_t mask = 1U << (page & 7);
    if (address & 4095U) return;
    if (used) page_bitmap[page >> 3] |= mask;
    else page_bitmap[page >> 3] &= (uint8_t)~mask;
}

/* End is exclusive; zero represents the end of the 4 GiB space. */
void set_bits(uint32_t start, uint32_t end, uint8_t used)
{
    uint32_t page, limit = end ? end >> 12 : PHYSICAL_PAGE_COUNT;
    for (page = start >> 12; page < limit; page++)
        set_bit(page << 12, used);
}

static uint32_t allocate_pages(uint32_t count)
{
    uint32_t scanned, run = 0, start = 0;
    /* All consumers currently require the kernel direct mapping. */
    uint32_t limit = allocation_limit;
    if (!physical_ready) return 0;
    if (!count || count >= limit) return 0;
    for (scanned = 0; scanned < limit; scanned++) {
        uint32_t page = next_page++;
        if (next_page >= limit) next_page = 1;
        if (page == 1) run = 0;
        if (!(page_bitmap[page >> 3] & (1U << (page & 7)))) {
            if (!run) start = page;
            if (++run == count) {
                set_bits(start << 12, (start + count) << 12, PAGE_USED);
                return start << 12;
            }
        } else run = 0;
    }
    return 0;
}

uint32_t alloc_pages(uint32_t count)
{
    uint32_t flags = mm_irq_save();
    uint32_t address = allocate_pages(count);
    mm_irq_restore(flags);
    return address;
}
uint32_t alloc_page(void) { return alloc_pages(1); }

void free_pages(uint32_t address, uint32_t count)
{
    if (!address || (address & 4095U) || !count ||
        count > PHYSICAL_PAGE_COUNT - (address >> 12)) return;
    uint32_t flags = mm_irq_save();
    set_bits(address, address + (count << 12), PAGE_FREE);
    mm_irq_restore(flags);
}

void free_page(uint32_t address) { free_pages(address, 1); }

void init_mem_regions(void)
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
        pa(kernel_virt_end) > 8U * 1024 * 1024) return -1;
    init_mem_regions();
    set_bits(PAGE_ALIGN_DOWN(task0_phys_start), PAGE_ALIGN_UP(task0_phys_end), PAGE_USED);
    for (i = 0; i < reservation_count; i++)
        set_bits(PAGE_ALIGN_DOWN(boot_reservations[i].start),
                 PAGE_ALIGN_UP(boot_reservations[i].end), PAGE_USED);
    for (i = 256; i < allocation_limit; i++)
        if (!(page_bitmap[i >> 3] & (1U << (i & 7)))) break;
    if (i == allocation_limit) return -1;
    next_page = i;
    physical_ready = 1;
    return 0;
}

void physical_memory_enable_direct_map(void)
{
    allocation_limit = DIRECT_MAP_SIZE >> 12;
}
