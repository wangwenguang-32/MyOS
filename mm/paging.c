#include <stdint.h>
#include <p_mem.h>
#include <printf.h>

#define PAGE_SIZE 4096           // 页大小：4096字节
#define PAGE_DIR_ENTRIES 1024    // 页目录项数
#define PAGE_TABLE_ENTRIES 1024  // 页表项数
#define VIRTUAL_ADDR_BITS 32     // 虚拟地址位数

// 页表项标志位
#define PAGE_PRESENT  0x01    // 页存在位
#define PAGE_WRITE    0x02    // 写权限位
#define PAGE_USER     0x04    // 用户模式位
#define PAGE_ACCESSED 0x20    // 访问位
#define PAGE_DIRTY    0x40    // 脏位

// 页表项结构（32位）
typedef uint32_t  page_entry_t;

// 页表结构
typedef struct {
    page_entry_t entries[PAGE_TABLE_ENTRIES];
} page_table_t;

// 页目录结构
typedef struct {
    page_entry_t* entries;
    uint32_t page_tables[PAGE_DIR_ENTRIES];
} page_directory_t;

// 虚拟地址结构
typedef struct {
    uint32_t page_dir_index : 10;  // 页目录索引 (10位)
    uint32_t page_table_index : 10; // 页表索引 (10位)
    uint32_t offset : 12;           // 页内偏移 (12位)
} virtual_addr_t;

extern uint32_t page_directory_base[];

page_directory_t  pdt;

// 从虚拟地址解析出各部分
void parse_virtual_address(uint32_t virtual_addr, virtual_addr_t *va) {
    va->page_dir_index = (virtual_addr >> 22) & 0x3FF;
    va->page_table_index = (virtual_addr >> 12) & 0x3FF;
    va->offset = virtual_addr & 0xFFF;
}


// 将虚拟地址映射到物理地址
int map_virtual_to_physical(uint32_t virtual_addr, 
                            uint32_t physical_addr, uint32_t flags) {
    virtual_addr_t va;
    parse_virtual_address(virtual_addr, &va);
    
    // 检查页目录索引是否有效
    if (va.page_dir_index >= PAGE_DIR_ENTRIES) {
        return -1;
    }
    
    // 如果页表不存在，创建它
    if (!pdt.entries[va.page_dir_index]) {
        
        // 分配页表的物理地址（简化处理）
        uint32_t page_table_phys = alloc_page();
        
        // 设置页目录项：指向页表
        pdt.entries[va.page_dir_index] =  page_table_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }
    
    // 检查页表索引是否有效
    if (va.page_table_index >= PAGE_TABLE_ENTRIES) {
        return -1;
    }
    
    // 设置页表项：指向物理页
    page_table_t *pt = (page_table_t *)pdt.page_tables[va.page_dir_index];
    pt->entries[va.page_table_index] = physical_addr | flags | PAGE_PRESENT;
    
    
    return 0;
}

// 查找虚拟地址对应的物理地址
int translate_virtual_to_physical(uint32_t virtual_addr, 
                                  uint32_t *physical_addr) {
    virtual_addr_t va;
    parse_virtual_address(virtual_addr, &va);
    
    // 检查页目录索引
    if (va.page_dir_index >= PAGE_DIR_ENTRIES) {
        return -1;
    }
    
    // 检查页目录项是否存在
    page_entry_t dir_entry = pdt.entries[va.page_dir_index];
    if (!(dir_entry & PAGE_PRESENT)) {
        return -1;
    }
    
    // 检查页表是否存在
    if (pdt.page_tables[va.page_dir_index]) {
        return -1;
    }
    
    // 检查页表索引
    if (va.page_table_index >= PAGE_TABLE_ENTRIES) {
        return -1;
    }
    
    // 获取页表项
    page_table_t *pt = (page_table_t *)pdt.page_tables[va.page_dir_index];
    page_entry_t page_entry = pt->entries[va.page_table_index];
    
    // 检查页表项是否存在
    if (!(page_entry & PAGE_PRESENT)) {
        return -1;
    }
    
    // 提取物理页地址（去掉低12位标志位）
    uint32_t phys_page = page_entry & 0xFFFFF000;
    
    // 加上页内偏移得到完整物理地址
    *physical_addr = phys_page + va.offset;
    
    return 0;
}

// 取消虚拟地址映射
int unmap_virtual_address(uint32_t virtual_addr) {
    virtual_addr_t va;
    parse_virtual_address(virtual_addr, &va);
    
    if (va.page_dir_index >= PAGE_DIR_ENTRIES || 
        va.page_table_index >= PAGE_TABLE_ENTRIES) {
        return -1;
    }
    
    if (pdt.page_tables[va.page_dir_index]) {
        return -1;
    }
    
    page_table_t *pt = (page_table_t *)pdt.page_tables[va.page_dir_index];
    
    // 清除页表项的存在位
    pt->entries[va.page_table_index] &= ~PAGE_PRESENT;
    
    
    return 0;
}

void _init_paging()
{
    pdt.entries=(page_entry_t*)((uint32_t)page_directory_base+0xC0000000);
    map_virtual_to_physical(0xFEE00000u,0xFEE00000u,6u);
    map_virtual_to_physical(0xFEC00000u,0xFEC00000u,6u);
}