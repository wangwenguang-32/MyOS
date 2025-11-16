#include <kmalloc.h>
#include <p_mem.h>
#include <page.h>
#include <stdint.h>
#include <printf.h>
#include <section.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// 从paging.c导入的函数声明
extern int map_virtual_to_physical(uint32_t virtual_addr, 
                                   uint32_t physical_addr, 
                                   uint32_t flags);
extern int translate_virtual_to_physical(uint32_t virtual_addr,
                                         uint32_t *physical_addr);
extern int unmap_virtual_address(uint32_t virtual_addr);

#define KERNEL_HEAP_START  0xD0000000  // 内核堆起始虚拟地址
#define KERNEL_HEAP_END    0xE0000000  // 内核堆结束虚拟地址
#define MIN_ALLOC_SIZE     16          // 最小分配大小（字节对齐）
#define BLOCK_HEADER_SIZE  8           // 块头部大小

// 内存块头部结构
typedef struct kmalloc_block {
    uint32_t size;                    // 块大小（不包括头部）
    uint32_t flags;                   // 标志位：0=空闲，1=已使用
    struct kmalloc_block* next;       // 下一个块（用于空闲链表）
} kmalloc_block_t;

// 当前堆指针
static uint32_t heap_current = KERNEL_HEAP_START;
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
void init_kmalloc(void) {
    heap_current = KERNEL_HEAP_START;
    free_list = NULL;
    printf("kmalloc initialized: heap at 0x%x - 0x%x\n", KERNEL_HEAP_START, KERNEL_HEAP_END);
}

// 分配新的页并映射
static void* alloc_new_page(void) {
    // 检查堆是否已满
    if (heap_current >= KERNEL_HEAP_END) {
        return NULL;
    }
    
    // 分配一个物理页
    uint32_t phys_addr = alloc_page();
    if (phys_addr == 0) {
        return NULL;  // 物理内存不足
    }
    
    // 将虚拟地址映射到物理地址
    uint32_t virt_addr = heap_current;
    if (map_virtual_to_physical(virt_addr, phys_addr, 
                                PAGE_PRESENT | PAGE_WRITE) != 0) {
        free_page(phys_addr);  // 映射失败，释放物理页
        return NULL;
    }
    
    heap_current += PAGE_SIZE;
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
    block->flags = 0;  // 标记为空闲
    block->next = free_list;
    free_list = block;
}

// 从空闲链表中移除块
static void remove_from_free_list(kmalloc_block_t* block) {
    kmalloc_block_t* current = free_list;
    kmalloc_block_t* prev = NULL;
    while (current != NULL) {
        if (current == block) {
            if (prev != NULL) {
                prev->next = current->next;
            } else {
                free_list = current->next;
            }
            break;
        }
        prev = current;
        current = current->next;
    }
}

// 合并相邻的空闲块
static void merge_free_blocks(kmalloc_block_t* block) {
    // 检查下一个块是否空闲且相邻
    kmalloc_block_t* next = (kmalloc_block_t*)((uint8_t*)block + BLOCK_HEADER_SIZE + block->size);
    if ((uint32_t)next < heap_current && next->flags == 0) {
        // 合并下一个块
        block->size += BLOCK_HEADER_SIZE + next->size;
        remove_from_free_list(next);
    }
    
    // 检查前一个块（需要遍历空闲链表找到前一个块）
    kmalloc_block_t* current = free_list;
    while (current != NULL) {
        kmalloc_block_t* current_next = (kmalloc_block_t*)((uint8_t*)current + BLOCK_HEADER_SIZE + current->size);
        if (current_next == block && current != block) {
            // 找到前一个相邻的空闲块，合并
            current->size += BLOCK_HEADER_SIZE + block->size;
            remove_from_free_list(block);
            block = current;  // 更新block为合并后的块
            break;
        }
        current = current->next;
    }
}

// 内核内存分配函数
void* kmalloc(uint32_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // 对齐到最小分配大小
    size = align_up(size, MIN_ALLOC_SIZE);
    
    // 如果请求的大小大于等于一页，直接分配整页
    if (size >= PAGE_SIZE - BLOCK_HEADER_SIZE) {
        uint32_t pages_needed = (size + BLOCK_HEADER_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
        uint32_t total_size = pages_needed * PAGE_SIZE;
        
        // 分配物理页
        uint32_t phys_addr = alloc_pages(pages_needed);
        if (phys_addr == 0) {
            return NULL;
        }
        
        // 分配虚拟地址空间
        if (heap_current + total_size > KERNEL_HEAP_END) {
            free_pages(phys_addr, pages_needed);
            return NULL;
        }
        
        // 映射每一页
        uint32_t virt_addr = heap_current;
        for (uint32_t i = 0; i < pages_needed; i++) {
            if (map_virtual_to_physical(virt_addr + i * PAGE_SIZE, 
                                       phys_addr + i * PAGE_SIZE,
                                       PAGE_PRESENT | PAGE_WRITE) != 0) {
                // 映射失败，释放已分配的页
                for (uint32_t j = 0; j < i; j++) {
                    unmap_virtual_address(virt_addr + j * PAGE_SIZE);
                }
                free_pages(phys_addr, pages_needed);
                return NULL;
            }
        }
        
        // 设置块头部
        kmalloc_block_t* block = (kmalloc_block_t*)virt_addr;
        block->size = total_size - BLOCK_HEADER_SIZE;
        block->flags = 1;  // 已使用
        block->next = NULL;
        
        heap_current += total_size;
        return (void*)(virt_addr + BLOCK_HEADER_SIZE);
    }
    
    // 小块内存分配：先尝试从空闲链表中查找
    kmalloc_block_t* block = find_free_block(size);
    if (block != NULL) {
        // 如果块太大，尝试分割
        if (block->size >= size + BLOCK_HEADER_SIZE + MIN_ALLOC_SIZE) {
            // 分割块
            kmalloc_block_t* new_block = (kmalloc_block_t*)((uint8_t*)block + BLOCK_HEADER_SIZE + size);
            new_block->size = block->size - size - BLOCK_HEADER_SIZE;
            new_block->flags = 0;
            new_block->next = NULL;
            add_to_free_list(new_block);
            block->size = size;
        }
        block->flags = 1;  // 标记为已使用
        return (void*)((uint8_t*)block + BLOCK_HEADER_SIZE);
    }
    
    // 空闲链表中没有合适的块，分配新页
    void* page = alloc_new_page();
    if (page == NULL) {
        return NULL;
    }
    
    // 在新页中创建块
    block = (kmalloc_block_t*)page;
    block->size = PAGE_SIZE - BLOCK_HEADER_SIZE;
    block->flags = 0;
    block->next = NULL;
    
    // 如果块太大，分割它
    if (block->size >= size + BLOCK_HEADER_SIZE + MIN_ALLOC_SIZE) {
        kmalloc_block_t* new_block = (kmalloc_block_t*)((uint8_t*)block + BLOCK_HEADER_SIZE + size);
        new_block->size = block->size - size - BLOCK_HEADER_SIZE;
        new_block->flags = 0;
        new_block->next = NULL;
        add_to_free_list(new_block);
        block->size = size;
    }
    
    block->flags = 1;  // 标记为已使用
    return (void*)((uint8_t*)block + BLOCK_HEADER_SIZE);
}

// 内核内存释放函数
void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    // 获取块头部
    kmalloc_block_t* block = (kmalloc_block_t*)((uint8_t*)ptr - BLOCK_HEADER_SIZE);
    
    if (block->flags == 0) {
        // 双重释放检测
        return;
    }
    
    // 检查是否是大块（整页分配）
    uint32_t block_size = block->size + BLOCK_HEADER_SIZE;
    if (block_size >= PAGE_SIZE) {
        // 大块：直接释放物理页
        uint32_t virt_addr = (uint32_t)block;
        uint32_t pages = (block_size + PAGE_SIZE - 1) / PAGE_SIZE;
        
        // 获取物理地址并释放
        uint32_t phys_addr;
        if (translate_virtual_to_physical(virt_addr, &phys_addr) == 0) {
            phys_addr = align_down(phys_addr, PAGE_SIZE);
            free_pages(phys_addr, pages);
        }
        
        // 取消映射
        for (uint32_t i = 0; i < pages; i++) {
            unmap_virtual_address(virt_addr + i * PAGE_SIZE);
        }
        
        return;
    }
    
    // 小块：添加到空闲链表
    block->flags = 0;
    add_to_free_list(block);
    
    // 尝试合并相邻的空闲块
    merge_free_blocks(block);
}

