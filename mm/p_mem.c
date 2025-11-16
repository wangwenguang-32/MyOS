#include<mmap.h>
#include<p_mem.h>
#include<printf.h>
#include<section.h>
#include<stdint.h>

extern uint32_t _boot_start;
extern uint32_t _boot_end;
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

#define sym_val(x)             ((uint32_t)&x)
#define pa(x)                     (x-0xC0000000)

uint32_t boot_kernel_start = sym_val(_boot_start);
uint32_t boot_kernel_end = sym_val(_boot_end);
uint32_t kernel_start = pa(sym_val(_kernel_start));
uint32_t kernel_end = pa(sym_val(_kernel_end));

uint8_t mem_page[2*1024*1024] DATA = {0};

#define PAGE_IN_USE  1   // 内存页正在使用
#define PAGE_FREE    0   // 内存页空闲


int32_t last_x=0;
int32_t last_y= -1;

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
   uint32_t group_count=(start_y+page_count)/8-1;

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
                    
                    uint32_t end_page_index = (end_addr >> 12);
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



void init_mem_page()
{
    uint32_t i=0;
    for(i;i<num_mmap;i++)
    {
        if(mmap[i].type==AddressRangeMemory)
        {
            continue;
        }
        unsigned long start_addr=PAGE_ALIGN_DOWN(mmap[i].base_addr);
        unsigned long end_addr=PAGE_ALIGN_UP(mmap[i].base_addr+mmap[i].length);
        set_bits(start_addr,end_addr,PAGE_IN_USE);
    }

    set_bits(boot_kernel_start,boot_kernel_end,PAGE_IN_USE);
    set_bits(kernel_start,kernel_end,PAGE_IN_USE);

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



void _init_mm()
{
    printf("boot_kernel_addr: 0x%x -- 0x%x\n",boot_kernel_start,boot_kernel_end);
    printf("kernel_addr:      0x%x -- 0x%x\n",kernel_start,kernel_end);
    init_mem_page();

}