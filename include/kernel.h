#ifndef  KERNEL_H
#define KERNEL_H

#include<stdint.h>

extern uint32_t boot_kernel_start;
extern uint32_t boot_kernel_end;
extern uint32_t task0_phys_start;
extern uint32_t task0_phys_end;
extern uint32_t kernel_virt_start;
extern uint32_t kernel_virt_end;

#define PAGE_OFFSET     0xC0000000U

#define sym_val(x)             ((uint32_t)&x)

#define pa(x)        ((uint32_t)x-PAGE_OFFSET)
#define va(x)        ((page_table_t *)((uint32_t)x+PAGE_OFFSET))



#define KERNEL_VIRT_BASE    0xC0000000UL

#define DIRECT_MAP_SIZE     (816UL  * 1024 * 1024)  /* 816MB 直接映射区 */
#define VMALLOC_SIZE        (128UL  * 1024 * 1024)  /* 128MB vmalloc区 */
#define IOREMAP_SIZE        (64UL   * 1024 * 1024)  /* 64MB  ioremap/MMIO区 */
#define FIXMAP_SIZE         (16UL   * 1024 * 1024)  /* 16MB  fixmap区 */

/*直接映射区 */
#define DIRECT_MAP_START    KERNEL_VIRT_BASE
#define DIRECT_MAP_END      (DIRECT_MAP_START + DIRECT_MAP_SIZE - 1)

/*vmalloc区 */
#define VMALLOC_START       (DIRECT_MAP_END + 1)
#define VMALLOC_END         (VMALLOC_START + VMALLOC_SIZE - 1)

/*ioremap / MMIO区 */
#define IOREMAP_START       (VMALLOC_END + 1)
#define IOREMAP_END         (IOREMAP_START + IOREMAP_SIZE - 1)

/*fixmap区*/
#define FIXMAP_START        (IOREMAP_END + 1)
#define FIXMAP_END          (FIXMAP_START + FIXMAP_SIZE - 1)


#define KERNEL_VIRT_END     0xFFFFFFFFUL

#endif