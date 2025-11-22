#ifndef  PAGE_H
#define PAGE_H
#include<stdint.h>

#define ALIGNED(n) __attribute__((aligned(n)))

typedef enum {
    PAGE_TYPE_NORMAL,       // 普通用户态/内核态数据页
    PAGE_TYPE_PAGE_TABLE,   // 页表本身占用的页（多级页表节点）
    PAGE_TYPE_DMA,          // 用于DMA的页（直接内存访问）
    PAGE_TYPE_KERNEL,       // 内核代码/数据页（如内核栈、全局变量）
    PAGE_TYPE_FILE_CACHE    // 文件缓存页（映射磁盘文件内容）
} PageType;

struct page  {
    uint32_t ref_count;
    PageType type;

} ALIGNED(8);

#define struct page page_t


#endif