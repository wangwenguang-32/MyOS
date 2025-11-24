#ifndef  PAGE_H
#define PAGE_H

#include <stdint.h>
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

#define PAGE_TABLE_MAP_SIZE  0x400000

// 页表项结构（32位）
typedef uint32_t  page_entry_t;

// 页表结构
typedef struct {
    page_entry_t entries[PAGE_TABLE_ENTRIES];
} page_table_t;

// 页目录结构
typedef struct {
    page_entry_t entries[PAGE_TABLE_ENTRIES];
} page_directory_t;

// 虚拟地址结构
typedef struct {
    uint32_t page_dir_index : 10;  // 页目录索引 (10位)
    uint32_t page_table_index : 10; // 页表索引 (10位)
    uint32_t offset : 12;           // 页内偏移 (12位)
} virtual_addr_t;

extern page_directory_t*  _pdt;

void parse_virtual_address(uint32_t virtual_addr, virtual_addr_t *va);
int map_virtual_to_physical(page_directory_t*pdt, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) ;
int translate_virtual_to_physical(page_directory_t*pdt,uint32_t virtual_addr, uint32_t *physical_addr) ;
int unmap_virtual_address(page_directory_t*pdt,uint32_t virtual_addr) ;
int map_virtual_range_to_physical(page_directory_t*pdt,uint32_t virtual_addr,uint32_t physical_addr,uint32_t page_count,uint32_t flags);
int set_page_entry(page_directory_t*pdt, uint32_t virtual_addr, page_entry_t entry);
int get_page_entry(page_directory_t*pdt, uint32_t virtual_addr, page_entry_t *entry);
int set_directory_entry(page_directory_t*pdt, uint32_t virtual_addr, page_entry_t entry);
int get_directory_entry(page_directory_t*pdt, uint32_t virtual_addr, page_entry_t *entry);

#endif