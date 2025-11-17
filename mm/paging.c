#include <stdint.h>
#include <p_mem.h>
#include <printf.h>
#include <page.h>
#include <section.h>


extern uint32_t page_directory_base[];

page_directory_t*  _pdt DATA;

// 从虚拟地址解析出各部分
void parse_virtual_address(uint32_t virtual_addr, virtual_addr_t *va) {
    va->page_dir_index = (virtual_addr >> 22) & 0x3FF;
    va->page_table_index = (virtual_addr >> 12) & 0x3FF;
    va->offset = virtual_addr & 0xFFF;
}


// 将虚拟地址映射到物理地址
int map_virtual_to_physical(page_directory_t*pdt, uint32_t virtual_addr, 
                            uint32_t physical_addr, uint32_t flags) {
    virtual_addr_t va;
    parse_virtual_address(virtual_addr, &va);
    
    if (va.page_dir_index >= PAGE_DIR_ENTRIES) {
        return -1;
    }
    
    if (!((pdt->entries[va.page_dir_index])&PAGE_PRESENT)) {
        uint32_t page_table_phys = alloc_page();
        pdt->entries[va.page_dir_index] =  page_table_phys | flags;
    }

    if (va.page_table_index >= PAGE_TABLE_ENTRIES) {
        return -1;
    }
    

    page_table_t *pt = (page_table_t *)((pdt->entries[va.page_dir_index])&0xFFFFF000u);
    pt->entries[va.page_table_index] = physical_addr | flags;
    
    
    return 0;
}

int map_virtual_range_to_physical(page_directory_t*pdt,uint32_t virtual_addr,uint32_t physical_addr,uint32_t page_count,uint32_t flags) {
    if ((virtual_addr & (PAGE_SIZE - 1)) ||
        (physical_addr & (PAGE_SIZE - 1)) ||
        page_count == 0) {
        return -1;
    }
    uint32_t i = 0;

    for (i; i < page_count; ++i) {
        uint32_t vaddr = virtual_addr + i * PAGE_SIZE;
        uint32_t paddr = physical_addr + i * PAGE_SIZE;
        if (map_virtual_to_physical(pdt,vaddr, paddr, flags) != 0) {
            return -1;
        }
    }

    return 0;
}

// 查找虚拟地址对应的物理地址
int translate_virtual_to_physical(page_directory_t*pdt,uint32_t virtual_addr, 
                                  uint32_t *physical_addr) {
    virtual_addr_t va;
    parse_virtual_address(virtual_addr, &va);
    
    if (va.page_dir_index >= PAGE_DIR_ENTRIES) {
        return -1;
    }
    
    page_entry_t dir_entry = pdt->entries[va.page_dir_index];
    if (!(dir_entry & PAGE_PRESENT)) {
        return -1;
    }
    
    if (!((pdt->entries[va.page_dir_index])&PAGE_PRESENT)) {
        return -1;
    }
    
    if (va.page_table_index >= PAGE_TABLE_ENTRIES) {
        return -1;
    }
    
    page_table_t *pt = (page_table_t *)((pdt->entries[va.page_dir_index])&0xFFFFF000u);
    page_entry_t page_entry = pt->entries[va.page_table_index];
    
    if (!(page_entry & PAGE_PRESENT)) {
        return -1;
    }
    
    uint32_t phys_page = page_entry & 0xFFFFF000;
    
    *physical_addr = phys_page + va.offset;
    
    return 0;
}

// 取消虚拟地址映射
int unmap_virtual_address(page_directory_t*pdt,uint32_t virtual_addr) {
    virtual_addr_t va;
    parse_virtual_address(virtual_addr, &va);
    
    if (va.page_dir_index >= PAGE_DIR_ENTRIES || 
        va.page_table_index >= PAGE_TABLE_ENTRIES) {
        return -1;
    }
    

     if (!((pdt->entries[va.page_dir_index])&PAGE_PRESENT)) {
        return -1;
    }
    
    page_table_t *pt = (page_table_t *)((pdt->entries[va.page_dir_index])&0xFFFFF000u);
    
    pt->entries[va.page_table_index] &= ~PAGE_PRESENT;
    
    
    return 0;
}

void _init_paging()
{
    _pdt=(page_directory_t*)((uint32_t)page_directory_base+0xC0000000);
    map_virtual_to_physical(_pdt,0xFEE00000u,0xFEE00000u,3u);
    map_virtual_to_physical(_pdt,0xFEC00000u,0xFEC00000u,3u);
}