#ifndef  VIRT_MEM_H
#define VIRT_MEM_H

#include <stdint.h>
#include <list.h>
#include <addr_translation.h>
#include <process.h>

// VMA (Virtual Memory Area) 标志位
#define VM_READ    0x01    // 可读
#define VM_WRITE   0x02    // 可写
#define VM_EXEC    0x04    // 可执行
#define VM_SHARED  0x08    // 共享内存
#define VM_PRIVATE 0x10    // 私有内存（写时复制）
#define VM_ANON    0x20    // 匿名映射
#define VM_FILE    0x40    // 文件映射
#define VM_GROWSDOWN 0x80  // 向下增长（栈）


// 用户空间地址范围（32位系统，3GB用户空间）
#define USER_SPACE_START  0x00000000
#define USER_SPACE_END    0xC0000000
#define USER_STACK_START  0xC0000000
#define USER_STACK_END    0xBFF00000 
#define USER_HEAP_START   0x08000000

// VMA 类型
typedef enum {
    VMA_TYPE_ANON,      // 匿名内存
    VMA_TYPE_FILE,      // 文件映射
    VMA_TYPE_STACK,     // 栈
    VMA_TYPE_HEAP,      // 堆
    VMA_TYPE_CODE,      // 代码段
    VMA_TYPE_DATA       // 数据段
} vma_type_t;

// 虚拟内存区域结构
typedef struct vm_area {
    uint32_t vm_start;      // 起始虚拟地址
    uint32_t vm_end;        // 结束虚拟地址（不包含）
    uint32_t vm_flags;      // 标志位（VM_READ, VM_WRITE等）
    vma_type_t vm_type;     // VMA 类型
    uint32_t vm_phys_addr;  // 物理地址（如果是文件映射，可能为0）
    uint32_t vm_file_offset;// 文件偏移（文件映射时使用）
    
    list_node_t vm_list;    // 链表节点，用于链接到进程的 VMA 链表
} vm_area_t;

// 进程内存描述符结构
typedef struct mm_struct {
    page_directory_t *pgd;          // 页目录指针
    list_node_t vm_area_list;       // VMA 链表头
    uint32_t vm_start;              // 进程虚拟地址空间起始
    uint32_t vm_end;                // 进程虚拟地址空间结束
    uint32_t code_start;            // 代码段起始
    uint32_t code_end;              // 代码段结束
    uint32_t data_start;            // 数据段起始
    uint32_t data_end;              // 数据段结束
    uint32_t heap_start;            // 堆起始
    uint32_t heap_end;              // 堆结束（brk指针）
    uint32_t stack_start;           // 栈起始
    uint32_t stack_end;             // 栈结束（栈顶）
    uint32_t vma_count;             // VMA 数量
} mm_struct_t;


void mm_destroy(mm_struct_t *mm);

vm_area_t *find_vma(mm_struct_t *mm, uint32_t addr);

vm_area_t *find_vma_intersection(mm_struct_t *mm, uint32_t start, uint32_t end);

vm_area_t *alloc_vma(void);

void free_vma(vm_area_t *vma);

int insert_vma(mm_struct_t *mm, vm_area_t *vma);

void remove_vma(mm_struct_t *mm, vm_area_t *vma);

void merge_vma(mm_struct_t *mm, vm_area_t *vma);

vm_area_t *vm_alloc(mm_struct_t *mm, uint32_t size, uint32_t flags, vma_type_t type);

int vm_free(mm_struct_t *mm, uint32_t addr, uint32_t size);

int vm_map_page(mm_struct_t *mm, uint32_t vaddr, uint32_t paddr, uint32_t flags);

int vm_unmap_page(mm_struct_t *mm, uint32_t vaddr);

int vm_map_range(mm_struct_t *mm, uint32_t vaddr, uint32_t paddr, uint32_t size, uint32_t flags);

int vm_unmap_range(mm_struct_t *mm, uint32_t vaddr, uint32_t size);

int mm_copy(mm_struct_t *dst_mm, mm_struct_t *src_mm);

int vm_brk(mm_struct_t *mm, uint32_t new_brk);

uint32_t vm_get_brk(mm_struct_t *mm);

int vm_check_range(mm_struct_t *mm, uint32_t addr, uint32_t size);

#endif 

