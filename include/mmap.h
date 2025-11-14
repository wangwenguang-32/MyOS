#ifndef  MMAP_H
#define MMAP_H

#define AddressRangeMemory         1  // 操作系统可用的内存
#define AddressRangeReserved       2 // 已占用/保留的内存
#define AddressRangeACPI           3  //ACPI解析后可用的内存
#define AddressRangeNVS            4  //ACPI NVS专用内存（不可分配）
#define AddressRangeUnusable       5 //硬件错误导致的不可用内存
#define AddressRangeDisabled       6 //未启用的内存区域

#define MAX_MAP_NUM   10

struct memory_map   //memory_region
{
    unsigned long base_addr;
    unsigned long length;
    unsigned int type;
};

extern struct memory_map mmap[MAX_MAP_NUM];
extern int num_mmap;

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
#endif
