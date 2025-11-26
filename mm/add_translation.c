#include<stdint.h>
#include<phy_mem.h>
#include<printf.h>
#include<addr_translation.h>
#include<section.h>
#include<kernel.h>
#include<memory.h>
#include<refcount.h>

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
        if(flags&PAGE_COW)
        {
            insert_count_node(page_table_phys);
        }
        pdt->entries[va.page_dir_index] =  page_table_phys | flags;
    }

    if (va.page_table_index >= PAGE_TABLE_ENTRIES) {
        return -1;
    }
    

    page_table_t *pt = (page_table_t *)((pdt->entries[va.page_dir_index])&0xFFFFF000u);
    va(pt)->entries[va.page_table_index] = physical_addr | flags;
    if(flags&PAGE_COW)
    {
        insert_count_node(physical_addr);
    }
    
    
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
    page_entry_t page_entry = va(pt)->entries[va.page_table_index];
    
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
    
    va(pt)->entries[va.page_table_index] &= ~PAGE_PRESENT;
    
    return 0;
}

int unmap_virtual_address_range(page_directory_t*pdt,uint32_t virtual_addr,uint32_t page_count) 
{
    uint32_t i = 0;
    for (i; i < page_count; ++i) 
    {
        uint32_t vaddr = virtual_addr + i * PAGE_SIZE;
        if (unmap_virtual_address(pdt,vaddr) != 0) 
        {
            return -1;
        }
    }

    return 0;
}


int set_virtual_range_readonly(page_directory_t *pdt, uint32_t start_vaddr, uint32_t size) {
    if (pdt == NULL || size == 0) {
        return -1;
    }

    uint32_t aligned_start = start_vaddr & ~(PAGE_SIZE - 1);
    uint32_t aligned_end = (start_vaddr + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    uint32_t dir_index_cached = PAGE_DIR_ENTRIES;
    page_entry_t cached_dir_entry = 0;
    uint32_t addr = aligned_start;
    for (addr; addr < aligned_end; addr += PAGE_SIZE) {
        virtual_addr_t va_desc;
        parse_virtual_address(addr, &va_desc);

        if (va_desc.page_dir_index >= PAGE_DIR_ENTRIES ||
            va_desc.page_table_index >= PAGE_TABLE_ENTRIES) {
            return -1;
        }

        page_entry_t dir_entry;
        if (va_desc.page_dir_index == dir_index_cached) {
            dir_entry = cached_dir_entry;
        } else {
            dir_entry = pdt->entries[va_desc.page_dir_index];
            dir_index_cached = va_desc.page_dir_index;
            cached_dir_entry = dir_entry;
        }

        if (!(dir_entry & PAGE_PRESENT)) {
            continue;
        }

        if (dir_entry & PAGE_WRITE) {
            dir_entry &= ~PAGE_WRITE;
            pdt->entries[va_desc.page_dir_index]=dir_entry;
            cached_dir_entry = dir_entry;
        }

        page_table_t *pt = (page_table_t *)(dir_entry & 0xFFFFF000u);
        page_entry_t page_entry = va(pt)->entries[va_desc.page_table_index];

        if (!(page_entry & PAGE_PRESENT)) {
            continue;
        }

        if (page_entry & PAGE_WRITE) {
            page_entry &= ~PAGE_WRITE;
            va(pt)->entries[va_desc.page_table_index]=page_entry;
        }

        __asm__ volatile("invlpg (%0);" ::"r"(addr):"memory");
    }

    return 0;
}

int copy_page_directory_range(page_directory_t *dst, page_directory_t *src, uint32_t start_vaddr, uint32_t size) {
    if (dst == NULL || src == NULL || size == 0) {
        return -1;
    }

    uint32_t aligned_start = start_vaddr & ~(PAGE_TABLE_MAP_SIZE - 1);
    uint64_t range_end = (uint64_t)start_vaddr + size;
    uint32_t aligned_end = (uint32_t)((range_end + PAGE_TABLE_MAP_SIZE - 1) & ~(PAGE_TABLE_MAP_SIZE - 1));

    uint32_t start_index = aligned_start / PAGE_TABLE_MAP_SIZE;
    uint32_t end_index = aligned_end / PAGE_TABLE_MAP_SIZE;

    if (start_index >= PAGE_DIR_ENTRIES || end_index > PAGE_DIR_ENTRIES) {
        return -1;
    }
    uint32_t idx = start_index;
    for (idx; idx < end_index; idx++) {
        page_entry_t src_entry = src->entries[idx];
        add_count(src_entry&0xFFFFF000u);
        dst->entries[idx]=src_entry;
    }

    return 0;
}

void kernel_direct_map_init()
{
    uint32_t count=DIRECT_MAP_SIZE>>12;
    map_virtual_range_to_physical(_pdt,0xC0000000,0x0,count,PAGE_PRESENT|PAGE_WRITE);

}

void _init_paging()
{
    uint8_t*addr=(uint8_t*)boot_kernel_start;
    uint16_t index=0;
    MEM_ZERO(addr,PAGE_SIZE);
    _pdt=(page_directory_t*)(boot_kernel_start+0xC0000000);
    kernel_direct_map_init();
    uint32_t phy_pdt=((uint32_t)_pdt)-0xC0000000;
    __asm__("movl %%eax,%%cr3;"::"a"(phy_pdt));
    
}