#include<mmap.h>
#include<p_mem.h>
#include<printf.h>

extern unsigned long _boot_start;
extern unsigned long _boot_end;
extern unsigned long _kernel_start;
extern unsigned long _kernel_end;

#define sym_val(x)             ((unsigned long)&x)
#define pa(x)                     (x-0xC0000000)

unsigned long boot_kernel_start = sym_val(_boot_start);
unsigned long boot_kernel_end = sym_val(_boot_end);
unsigned long kernel_start = pa(sym_val(_kernel_start));
unsigned long kernel_end = pa(sym_val(_kernel_end));

unsigned char mem_page[2*1024*1024] __attribute__((section(".data.mem_page")));

#define USABLE    1
#define UNUSABLE  0

int last_x=0;
int last_y=-1;

void set_bit(unsigned long addr,unsigned char type)
{
    if((addr&0xFFF)!=0)
        return;
    unsigned int index1=(addr>>12)/8;
    unsigned char index2 = (addr>>12)%8;
    switch (type)
    {
        case USABLE:
        {
            mem_page[index1]=mem_page[index1] | (1<<index2);
            break;
        }
        case UNUSABLE:
        {
            mem_page[index1]=mem_page[index1] & (~(1<<index2));
            break;
        }
    }

}

void set_bits(unsigned long start_addr,unsigned long end_addr,char type)
{
   int start_x=(start_addr>>12)/8;
   int start_y=(start_addr>>12)%8;

   int page_count=(end_addr>>12)-(start_addr>>12);
   int group_count=(start_y+page_count)/8-1;

   int start_count= (start_y+page_count)>8 ? (8-start_y) : page_count;
   int end_count=(start_y+page_count)>8 ? (start_y+page_count)%8 :0;

   switch (type)
   {
   case USABLE:
   {
        mem_page[start_x] |= (((1U<<start_count)-1)<<(8-start_y-start_count) );
        start_x++;
        int i=0;
        for(i;group_count>0,i<group_count;i++,start_x++)
        {
            mem_page[start_x]=0xFF;
        }
        mem_page[start_x] |= (((1U<<end_count)-1)<<(8-end_count));
   }
    case UNUSABLE:
   {
        mem_page[start_x] &= ~(((1U<<start_count)-1)<<(8-start_y-start_count) );
        start_x++;
        int i=0;
        for(i;group_count>0,i<group_count;i++,start_x++)
        {
            mem_page[start_x]=0x00;
        }
        mem_page[start_x] &= ~(((1U<<end_count)-1)<<(8-end_count));

   }
   }

}

void init_mem_page()
{
    int i=0;
    for(i;i<num_mmap;i++)
    {
        if(mmap[i].type==AddressRangeMemory)
        {
            continue;
        }
        unsigned long start_addr=PAGE_ALIGN_DOWN(mmap[i].base_addr);
        unsigned long end_addr=PAGE_ALIGN_UP(mmap[i].base_addr+mmap[i].length);
        set_bits(start_addr,end_addr,USABLE);

    }

    set_bits(PAGE_ALIGN_DOWN(boot_kernel_start),PAGE_ALIGN_UP(boot_kernel_end),USABLE);
    set_bits(PAGE_ALIGN_DOWN(kernel_start),PAGE_ALIGN_UP(kernel_end),USABLE);


}

unsigned long alloc_page()
{
    int x=last_x;
    int y=last_y;
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
                set_bit(addr,USABLE);
                return addr;
            }
        }
    }

}

void free_page(unsigned long addr)
{
    set_bit(addr,UNUSABLE);

}


void _init_mm()
{
    char* addr=mem_page;
    int i=0;
    for(i;i<0x200000;i++)
    {
        *(addr+i)=0;
    }
    init_mem_page();

}