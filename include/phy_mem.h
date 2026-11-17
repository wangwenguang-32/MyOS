#ifndef  P_MEM_H
#define P_MEM_H

#include<stdint.h>

#define AddressRangeMemory         1  // 操作系统可用的内存
#define AddressRangeReserved       2 // 已占用/保留的内存
#define AddressRangeACPI           3  //ACPI解析后可用的内存
#define AddressRangeNVS            4  //ACPI NVS专用内存（不可分配）
#define AddressRangeUnusable       5 //硬件错误导致的不可用内存
#define AddressRangeDisabled       6 //未启用的内存区域

#define MAX_MAP_NUM   20

struct memory_region   //memory_region
{
    uint64_t base_addr;
    uint64_t length;
    unsigned int type;
};

static inline const char* addr_range_type_str(unsigned type) {
    switch (type) {
        case AddressRangeMemory: return "usable";
        case AddressRangeReserved: return "reserved";
        case AddressRangeACPI: return "acpi";
        case AddressRangeNVS: return "nvs";
        case AddressRangeUnusable: return "unusable";
        case AddressRangeDisabled: return "disabled";
        default: return "invalid memory type";
    }
}

#define PAGE_ALIGN_DOWN(addr) ((addr) & ~0xFFFU)
#define PAGE_ALIGN_UP(addr)   (((addr) + 0xFFFU) & ~0xFFFU)

uint32_t alloc_page();
void free_page(uint32_t addr);

uint32_t alloc_pages(uint32_t count);
void free_pages(uint32_t start_addr, uint32_t count);

struct multiboot_tag_mmap;
void setup_mmap(struct multiboot_tag_mmap *tag);
void _init_phy_mem(void);

#endif