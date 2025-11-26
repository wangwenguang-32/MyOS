#include<refcount.h>
#include<stdint.h>
#include<kmalloc.h>
#include<printf.h>

LIST_HEAD(phys_mem_count_head);


void insert_count_node(uint32_t addr)
{
    phys_mem_refcount_t*mem_count=(phys_mem_refcount_t*)kmalloc(sizeof(phys_mem_refcount_t));
    addr&=0xFFFFF000u;
    mem_count->address_count=addr|1u;
    list_add_tail(&mem_count->node,&phys_mem_count_head);
}

uint8_t add_count(uint32_t addr)
{
    phys_mem_refcount_t*tmp;
    list_for_each_entry(tmp,&phys_mem_count_head,node)
    {
        if((tmp->address_count&0xFFFFF000u)==addr)
        {
            if((tmp->address_count&0xFFFu)<4096)
            {
                tmp->address_count+=1;
                return 1;
            }
            break;
        }
    }
    return 0;
}

uint8_t del_count(uint32_t addr)
{
    phys_mem_refcount_t*tmp;
    list_for_each_entry(tmp,&phys_mem_count_head,node)
    {
        if((tmp->address_count&0xFFFFF000u)==addr)
        {
            if((tmp->address_count&0xFFFu)>1)
            {
                tmp->address_count-=1;
                return 1;
            }
            break;
        }
    }
    return 0;
}

uint8_t get_count(uint32_t addr)
{
    phys_mem_refcount_t*tmp;
    list_for_each_entry(tmp,&phys_mem_count_head,node)
    {
        if((tmp->address_count&0xFFFFF000u)==addr)
        {
            uint32_t count=tmp->address_count&0xFFFu;
            return count;
        }
    }
    return 0;
}


