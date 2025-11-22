#include <kmalloc.h>
#include <phy_mem.h>
#include <addr_translation.h>
#include <stdint.h>
#include <printf.h>
#include <section.h>

#ifndef NULL
#define NULL ((void*)0)
#endif




#define KERNEL_HEAP_SIZE_INIT  0x40000
#define KERNEL_HEAP_PAGE_COUNT  (KERNEL_HEAP_SIZE_INIT>>12)

#define MIN_ALLOC_SIZE     8         
#define BLOCK_HEADER_SIZE  8 

#define USE  1
#define FREE 0

typedef struct kmalloc_block {
    uint32_t size;                    // 块大小（不包括头部)
    struct kmalloc_block* next;       // 下一个块（用于空闲链表）
} kmalloc_block_t;

static kmalloc_block_t* free_list = NULL;

// 对齐到指定边界
static inline uint32_t align_up(uint32_t addr, uint32_t align) {
    return (addr + align - 1) & ~(align - 1);
}

// 对齐到指定边界（向下）
static inline uint32_t align_down(uint32_t addr, uint32_t align) {
    return addr & ~(align - 1);
}

// 初始化kmalloc分配器
void _init_kmalloc(void) {
    uint32_t p_heap_addr_start=alloc_pages(KERNEL_HEAP_PAGE_COUNT);
    uint32_t v_heap_addr_start=p_heap_addr_start+0xC0000000;
    //map_virtual_range_to_physical(_pdt,v_heap_addr_start,p_heap_addr_start,KERNEL_HEAP_PAGE_COUNT,PAGE_PRESENT|PAGE_WRITE);
    free_list=(kmalloc_block_t*)v_heap_addr_start;
    uint32_t size =KERNEL_HEAP_SIZE_INIT<<8;
    free_list->size=size;
    free_list->next=NULL;
    printf("kmalloc initialized: heap at 0x%x \n", v_heap_addr_start);
}

static void* alloc_new_page(void) {
    uint32_t phys_addr = alloc_page();
    if (phys_addr == 0) {
        return NULL;  
    }
    
    uint32_t virt_addr = phys_addr+0xC0000000;
    if (map_virtual_to_physical(_pdt,virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITE) != 0) 
    {
        free_page(phys_addr); 
        return NULL;
    }
    return (void*)virt_addr;
}

// 在空闲链表中查找合适的块
static kmalloc_block_t* find_free_block(uint32_t size) {
    kmalloc_block_t* current = free_list;
    kmalloc_block_t* prev = NULL;
    
    while (current != NULL) {
        if (current->size >= size) {
            // 找到合适的块
            if (prev != NULL) {
                prev->next = current->next;
            } else {
                free_list = current->next;
            }
            return current;
        }
        prev = current;
        current = current->next;
    }
    
    return NULL;
}

// 将块添加到空闲链表
static void add_to_free_list(kmalloc_block_t* block) {
    if(free_list==NULL)
    {
        free_list=block;
        return;
    }
    if(free_list->size>=block->size)
    {
        block->next=free_list;
        free_list=block;
        return;
    }
    kmalloc_block_t*tmp=free_list;
    while(tmp->next!=NULL)
    {
        if(tmp->size>=block->size&&tmp->size<=block->size)
        {
            block->next=tmp->next;
            tmp->next=block;
            return;
        }
        tmp=tmp->next;
    }

    tmp->next=block;
    
}


// 合并相邻的空闲块
static kmalloc_block_t* merge_free_blocks(kmalloc_block_t* block) 
{
    
    kmalloc_block_t* prev=NULL;
    kmalloc_block_t* next = (kmalloc_block_t*)((uint8_t*)block + BLOCK_HEADER_SIZE + block->size);
    uint8_t have_next=0;
    kmalloc_block_t*tmp=free_list;
    kmalloc_block_t*tmp_prev=NULL;
    while(tmp->next!=NULL)
    {
        if(tmp->size+(uint32_t)tmp==(uint32_t)block)
        {
            prev=tmp;
            if(tmp_prev==NULL)
            {
                free_list=tmp->next;
            }
            else
            {
                tmp_prev=tmp->next;
            }
            if(have_next)
            {
                break;
            }
        }
        else if(next==tmp)
        {
             if(tmp_prev==NULL)
            {
                free_list=tmp->next;
            }
            else
            {
                tmp_prev=tmp->next;
            }
            if(prev!=NULL)
            {
                break;
            }
        }
        else
        {
            tmp_prev=tmp;
            tmp=tmp->next;
        }

    }
    if(prev!=NULL)
    {
        prev->size=prev->size+BLOCK_HEADER_SIZE+block->size;
        if(have_next)
        {
            prev->size=prev->size+BLOCK_HEADER_SIZE+next->size;
        }
        return prev;
    }
    else
    {
         if(have_next)
        {
            block->size=block->size+BLOCK_HEADER_SIZE+next->size;
        }
        return block;
    }

    return NULL;
    
}

// 内核内存分配函数
uint32_t kmalloc(uint32_t size) {
    if (size == 0) {
        return 0;
    }
    
    size = align_up(size, MIN_ALLOC_SIZE);
    
    kmalloc_block_t* block = find_free_block(size);
    printf("block:%x\n",block);
    if (block != NULL) {
        if (block->size >= size + BLOCK_HEADER_SIZE) {
            kmalloc_block_t* new_block = (kmalloc_block_t*)((uint8_t*)block + BLOCK_HEADER_SIZE + size);
            new_block->size = block->size - size - BLOCK_HEADER_SIZE;
            new_block->next = NULL;
            add_to_free_list(new_block);
            block->size = size;
        }
        return (uint32_t)((uint8_t*)block + BLOCK_HEADER_SIZE);
    }
    
    void* page = alloc_new_page();
    if (page == NULL) {
        return 0;
    }
    
    block = (kmalloc_block_t*)page;
    block->size = PAGE_SIZE - BLOCK_HEADER_SIZE;
    block->next = NULL;
    
    if (block->size >= size + BLOCK_HEADER_SIZE + MIN_ALLOC_SIZE) {
        kmalloc_block_t* new_block = (kmalloc_block_t*)((uint8_t*)block + BLOCK_HEADER_SIZE + size);
        new_block->size = block->size - size - BLOCK_HEADER_SIZE;
        new_block->next = NULL;
        add_to_free_list(new_block);
        block->size = size;
    }
    
    return (uint32_t)((uint8_t*)block + BLOCK_HEADER_SIZE);
}

// 内核内存释放函数
void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    kmalloc_block_t* block = (kmalloc_block_t*)((uint8_t*)ptr - BLOCK_HEADER_SIZE);
    kmalloc_block_t*new= merge_free_blocks(block);
    printf("free %x\n",new);
    if(new!=NULL)
    {
        add_to_free_list(new);
    }
    else
    {
        add_to_free_list(block);
    }
}

