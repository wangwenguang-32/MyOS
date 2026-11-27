#include <printf.h>
#include <process.h>
#include <stdint.h>
#include <paging.h>
#include <physical_memory.h>
#include <kernel.h>
#include <page_refcount.h>

#define PF_ERR_PRESENT       (1 << 0)  // Bit0：页存在位（0=页不存在，1=页存在但权限错误）
#define PF_ERR_WRITE         (1 << 1)  // Bit1：写操作位（0=读/执行操作，1=写操作）
#define PF_ERR_USER          (1 << 2)  // Bit2：用户态位（0=内核态访问，1=用户态访问）
#define PF_ERR_RESERVED      (1 << 3)  // Bit3：保留位违规（0=无，1=访问了保留位地址）
#define PF_ERR_INSTRUCTION   (1 << 4)  // Bit4：指令获取位（0=数据访问，1=取指令时异常）

void copy_page(uint8_t*src_addr,uint8_t*dst_addr)
{
    uint16_t i=0;
    for(i;i<PAGE_SIZE;i++)
    {
        *(dst_addr+i)=*(src_addr+i);
    }
}


void handle_user_write_prot_fault(unsigned long fault_addr)
{
    page_directory_t*pdt=current->mm->pgd;
    virtual_addr_t va;
    parse_virtual_address(fault_addr,&va);
    page_entry_t directory_entry=pdt->entries[va.page_dir_index];
    if((directory_entry&PAGE_WRITE)==0)
    {
        uint32_t new_page_table_phys=(uint32_t)alloc_page();
        uint32_t old_page_table_phys=(uint32_t)(directory_entry&0xFFFFF000u);
        if(get_count(old_page_table_phys)==1)
        {
            pdt->entries[va.page_dir_index]|=PAGE_WRITE;
        }
        else
        {
            copy_page(old_page_table_phys+PAGE_OFFSET,new_page_table_phys+PAGE_OFFSET);
            add_count((va(old_page_table_phys)->entries[va.page_table_index])&0xFFFFF000u);
            insert_count_node(new_page_table_phys);
            del_count(old_page_table_phys);
            pdt->entries[va.page_dir_index]&=0xFFFu;
            pdt->entries[va.page_dir_index]|=new_page_table_phys;
            pdt->entries[va.page_dir_index]|=PAGE_WRITE;
        }

    }

    directory_entry=pdt->entries[va.page_dir_index];
    page_table_t *pt = (page_table_t *)(directory_entry & 0xFFFFF000u);
    page_entry_t page_entry = va(pt)->entries[va.page_table_index];
    if((page_entry&PAGE_WRITE)==0)
    {
        uint32_t new_addr_phys=(uint32_t)alloc_page();
        uint32_t old_addr_phys=(uint32_t)(page_entry&0xFFFFF000u);
        if(get_count(old_addr_phys)==1)
        {
            va(pt)->entries[va.page_table_index]|=PAGE_WRITE;
        }
        else
        {
            copy_page(old_addr_phys+PAGE_OFFSET,new_addr_phys+PAGE_OFFSET);
            insert_count_node(new_addr_phys);
            del_count(old_addr_phys);
            va(pt)->entries[va.page_table_index]&=0xFFFu;
            va(pt)->entries[va.page_table_index]|=new_addr_phys;
            va(pt)->entries[va.page_table_index]|=PAGE_WRITE;
        }

    }
    __asm__ volatile("invlpg (%0);" ::"r"(fault_addr):"memory");


}


/* frame points to CPU-saved EIP, CS, EFLAGS (and user ESP/SS at CPL3). */
void do_page_fault(uint32_t error_code, const uint32_t *frame)
{
    uint32_t address, cr3, pde = 0, pte = 0;
    __asm__ volatile("movl %%cr2, %0" : "=r"(address));
    __asm__ volatile("movl %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("cli" ::: "memory");
    printf("\n[PAGE FAULT] CR2=0x%x error=0x%x CR3=0x%x\n", address, error_code, cr3);
    printf("EIP=0x%x CS=0x%x EFLAGS=0x%x pid=%d\n",
           frame[0], frame[1], frame[2], current ? current->pid : -1);
    printf("present=%u write=%u user=%u reserved=%u fetch=%u\n",
           error_code & 1, (error_code >> 1) & 1, (error_code >> 2) & 1,
           (error_code >> 3) & 1, (error_code >> 4) & 1);
    if ((cr3 & ~4095U) < DIRECT_MAP_SIZE) {
        page_directory_t *directory = (void *)((cr3 & ~4095U) + PAGE_OFFSET);
        pde = directory->entries[address >> 22];
        if ((pde & PAGE_PRESENT) && !(pde & 0x80U) && (pde & ~4095U) < DIRECT_MAP_SIZE) {
            page_table_t *table = (void *)((pde & ~4095U) + PAGE_OFFSET);
            pte = table->entries[(address >> 12) & 1023U];
        }
    }
    printf("PDE=0x%x PTE=0x%x\n", pde, pte);
    for (;;) __asm__ volatile("hlt");
}
