#include<phy_mem.h>
#include<printf.h>
#include<section.h>
#include<stdint.h>
#include<multiboot2.h>
#include<kernel.h>



uint8_t mem_page[2*1024*1024] DATA = {0};

#define PAGE_IN_USE  1   // 内存页正在使用
#define PAGE_FREE    0   // 内存页空闲


int32_t last_x=0;
int32_t last_y= -1;
struct memory_region  mem_regions[MAX_MAP_NUM];
int num_region=0;

void setup_mmap(struct multiboot_tag_mmap *tag)
{
            multiboot_memory_map_t *map;
            
            for (map = ((struct multiboot_tag_mmap *) tag)->entries;
                 (multiboot_uint8_t *) map 
                   < (multiboot_uint8_t *) tag + tag->size;
                 map = (multiboot_memory_map_t *) 
                   ((unsigned long) map
                    + ((struct multiboot_tag_mmap *) tag)->entry_size))
                    {
                      printf (" [base_addr = 0x%x%x, length = 0x%x%x] %s\n",
                      (unsigned) (map->addr >> 32),
                      (unsigned) (map->addr & 0xffffffff),
                      (unsigned) (map->len >> 32),
                      (unsigned) (map->len & 0xffffffff),addr_range_type_str(map->type));
                      mem_regions[num_region].base_addr= (map->addr & 0xffffffff);
                      mem_regions[num_region].length= (map->len & 0xffffffff);
                      mem_regions[num_region].type=map->type;
                      num_region++;
                    }
            
              

}


void set_bit(uint32_t addr,uint8_t type)
{
    if((addr&0xFFF)!=0)
        return;
    uint32_t index1=(addr>>12)/8;
    uint8_t index2 = (addr>>12)%8;
    switch (type)
    {
        case PAGE_IN_USE:
        {
            mem_page[index1]=mem_page[index1] | (1<<index2);
            break;
        }
        case PAGE_FREE:
        {
            mem_page[index1]=mem_page[index1] & (~(1<<index2));
            break;
        }
    }

}

void set_bits(uint32_t start_addr,uint32_t end_addr,uint8_t type)
{
   uint32_t end_index=0;
   if(end_addr==0x0)
   {
    end_index=0x100000;
   }
   else
   {
    end_index=end_addr>>12;
   }
   uint32_t start_x=(start_addr>>12)/8;
   uint32_t start_y=(start_addr>>12)%8;

   uint32_t page_count=end_index-(start_addr>>12);
   uint32_t group_count;
   if(start_y+page_count<8)
   {
    group_count=0;
   }
   else
   {
    group_count=(start_y+page_count)/8-1;
   }

   uint32_t start_count= (start_y+page_count)>8 ? (8-start_y) : page_count;
   uint32_t end_count=(start_y+page_count)>8 ? (start_y+page_count)%8 :0;

   switch (type)
   {
   case PAGE_IN_USE:
   {
        mem_page[start_x] |= (((1U<<start_count)-1)<<(8-start_y-start_count) );
        start_x++;
        uint32_t i=0;
        for(i;group_count>0,i<group_count;i++,start_x++)
        {
            mem_page[start_x]=0xFF;
        }
        mem_page[start_x] |= (((1U<<end_count)-1)<<(8-end_count));
        break;
   }
    case PAGE_FREE:
   {
        mem_page[start_x] &= ~(((1U<<start_count)-1)<<(8-start_y-start_count) );
        start_x++;
        uint32_t i=0;
        for(i;group_count>0,i<group_count;i++,start_x++)
        {
            mem_page[start_x]=0x00;
        }
        mem_page[start_x] &= ~(((1U<<end_count)-1)<<(8-end_count));
        break;

   }
   }

}

static int check_pages_free(uint32_t start_addr, uint32_t count)
{
    uint32_t i;
    for(i = 0; i < count; i++)
    {
        uint32_t addr = start_addr + (i << 12);
        uint32_t index1 = (addr >> 12) / 8;
        uint8_t index2 = (addr >> 12) % 8;
        
        if((mem_page[index1] & (1U << index2)) != 0)
        {
            return 0;
        }
    }
    return 1;
}

uint32_t alloc_pages(uint32_t count)
{
    if(count == 0)
    {
        return 0;
    }
    
    int32_t x = last_x;
    int32_t y = last_y;
    
    for(last_y++; (last_x != x || last_y != y); last_x++)
    {
        if(last_x == 0x200000)
        {
            last_x = 0;
        }
        if(last_y == 8)
        {
            last_y = 0;
            continue;
        }
        
        if(mem_page[last_x] == 0xFF)
        {
            last_y = 0;
            continue;
        }
        
        for(; last_y < 8; last_y++)
        {
            if((mem_page[last_x] & (1U << last_y)) == 0)
            {
                uint32_t start_addr = (last_x * 8 + last_y) << 12;
                if(check_pages_free(start_addr, count))
                {
                    uint32_t end_addr = start_addr + (count << 12);
                    set_bits(start_addr, end_addr, PAGE_IN_USE);
                    
                    uint32_t end_page_index = (end_addr >> 12)-1;
                    last_x = end_page_index / 8;
                    last_y = end_page_index % 8;
                    
                    return start_addr;
                }
            }
        }
        last_y = 0;
    }
    
    return 0; 
}

void free_pages(uint32_t start_addr, uint32_t count)
{
    if(count == 0 || (start_addr & 0xFFF) != 0)
    {
        return; 
    }
    
    uint32_t end_addr = start_addr + (count << 12);
    set_bits(start_addr, end_addr, PAGE_FREE);
}



void init_mem_regions()
{
    uint32_t i=0;
    for(i;i<num_region;i++)
    {
        
        if(mem_regions[i].type==AddressRangeMemory)
        {
            continue;
        }
        unsigned long start_addr=PAGE_ALIGN_DOWN(mem_regions[i].base_addr);
        unsigned long end_addr=PAGE_ALIGN_UP(mem_regions[i].base_addr+mem_regions[i].length);
        set_bits(start_addr,end_addr,PAGE_IN_USE);
    }

    set_bits(boot_kernel_start,boot_kernel_end,PAGE_IN_USE);
    set_bits(pa(kernel_virt_start),pa(kernel_virt_end),PAGE_IN_USE);

    set_bits(0xA0000,0xC0000,PAGE_IN_USE);


}

uint32_t alloc_page()
{
    int32_t x=last_x;
    int32_t y=last_y;
    for(last_y++;(last_x!=x||last_y!=y);last_x++)
    {
        if(mem_page[last_x]==0xFF)
        {
            last_y=0;
            continue;
        }
        if(last_x==0x200000)
        {
            last_x=0;
        }
        if(last_y==8)
        {
            last_y=0;
            continue;
        }
        for(last_y;last_y<8;last_y++)
        {
            if((mem_page[last_x]&(1U<<last_y))==0)
            {
                
                unsigned long addr=(last_x*8+last_y)<<12;
                set_bit(addr,PAGE_IN_USE);
                return addr;
            }
        }
    }

}

void free_page(uint32_t addr)
{
    set_bit(addr,PAGE_FREE);

}



void _init_phy_mem()
{
    uint32_t last_alloc_addr=kernel_virt_end-0x1000-0xC0000000;
    last_x=(last_alloc_addr>>12)/8;
    last_y=(last_alloc_addr>>12)%8;

    printf("boot_kernel_addr:0x%x -- 0x%x\n",boot_kernel_start,boot_kernel_end);
    printf("task0_phys_addr:0x%x -- 0x%x\n",task0_phys_start,task0_phys_end);
    printf("kernel_virt_addr:0x%x -- 0x%x\n",kernel_virt_start,kernel_virt_end);
    init_mem_regions();

}