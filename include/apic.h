#ifndef APIC_H
#define APIC_H

#include <stdint.h>


/* -------------------- Constants -------------------- */
#define IA32_APIC_BASE_MSR       0x1B
#define IA32_APIC_BASE_ENABLE    (1u << 11)
#define IA32_X2APIC_ENABLE       (1u << 10)

#define LAPIC_DEFAULT_PHYS       0xFEE00000u
#define IOAPIC_DEFAULT_PHYS      0xFEC00000u

/* LAPIC MMIO offsets */
#define LAPIC_ID                 0x020
#define LAPIC_TPR                0x080
#define LAPIC_EOI                0x0B0
#define LAPIC_SVR                0x0F0
#define LAPIC_LVT_TIMER          0x320
#define LAPIC_LVT_THERMAL        0x330
#define LAPIC_LVT_PERF           0x340
#define LAPIC_LVT_LINT0          0x350
#define LAPIC_LVT_LINT1          0x360
#define LAPIC_LVT_ERROR          0x370
#define LAPIC_TIMER_INIT_CNT     0x380
#define LAPIC_TIMER_CUR_CNT      0x390
#define LAPIC_TIMER_DIV_CFG      0x3E0
/* IOAPIC indirect regs */
#define IOAPIC_REGSEL            0x00
#define IOAPIC_WINDOW            0x10
#define IOAPIC_REG_ID            0x00
#define IOAPIC_REG_VER           0x01
#define IOAPIC_REG_REDTBL        0x10

/* PIC ports */
#define PIC1_CMD 0x20
#define PIC1_DAT 0x21
#define PIC2_CMD 0xA0
#define PIC2_DAT 0xA1

// 系统核心设备
#define IRQ_TIMER        0   // 系统定时器（PIT），IRQ0
#define IRQ_KEYBOARD     1   // 键盘控制器，IRQ1
#define IRQ_CASCADE      2   // 主从PIC级联中断，IRQ2（通常不直接关联设备）
#define IRQ_COM2         3   // 串行端口2，IRQ3
#define IRQ_COM1         4   // 串行端口1，IRQ4
#define IRQ_LPT2         5   // 并行端口2/声卡，IRQ5
#define IRQ_FLOPPY       6   // 软盘控制器，IRQ6
#define IRQ_LPT1         7   // 并行端口1/打印机，IRQ7

// 扩展设备
#define IRQ_RTC          8   // 实时时钟（RTC），IRQ8
#define IRQ_REDIRECT_IRQ2 9  // IRQ2重定向（兼容用途），IRQ9
#define IRQ_USB1         10  // 通用USB控制器1，IRQ10（预留）
#define IRQ_USB2         11  // 通用USB控制器2/显卡，IRQ11（预留）
#define IRQ_MOUSE        12  // PS/2鼠标控制器，IRQ12
#define IRQ_FPU_ERROR    13  // 浮点运算单元错误，IRQ13
#define IRQ_IDE_PRIMARY  14  // 主IDE硬盘控制器，IRQ14
#define IRQ_IDE_SECONDARY 15 // 从IDE硬盘控制器，IRQ15


/* -------------------- MMIO helpers -------------------- */
static volatile uint32_t *const lapic_base = (volatile uint32_t *)(uintptr_t)LAPIC_DEFAULT_PHYS;
static volatile uint32_t *const ioapic_base = (volatile uint32_t *)(uintptr_t)IOAPIC_DEFAULT_PHYS;

static inline void lapic_write(uint32_t reg, uint32_t val) {
    *(volatile uint32_t *)((uintptr_t)lapic_base + reg) = val;
    (void)*(volatile uint32_t *)((uintptr_t)lapic_base + LAPIC_ID);
}

static inline uint32_t lapic_read(uint32_t reg) {
    return *(volatile uint32_t *)((uintptr_t)lapic_base + reg);
}

static inline void ioapic_write(uint8_t reg, uint32_t val) {
    *(volatile uint32_t *)((uintptr_t)ioapic_base + IOAPIC_REGSEL) = reg;
    *(volatile uint32_t *)((uintptr_t)ioapic_base + IOAPIC_WINDOW) = val;
}

static inline uint32_t ioapic_read(uint8_t reg) {
    *(volatile uint32_t *)((uintptr_t)ioapic_base + IOAPIC_REGSEL) = reg;
    return *(volatile uint32_t *)((uintptr_t)ioapic_base + IOAPIC_WINDOW);
}


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize local APIC and IOAPIC.
 * spurious_vector: vector number for LAPIC SVR (bit8 software enable is set internally).
 * ext_irq_vector_base: base vector for legacy external IRQs (e.g., 0x20 for IRQ0..15 → 0x20..0x2F).
 */
void _apic_init(uint8_t spurious_vector, uint8_t ext_irq_vector_base);
/* Send End-Of-Interrupt to LAPIC. Call at end of external IRQ handlers. */
void lapic_eoi(void);

void lapic_mask_interrupt(uint32_t reg);
void lapic_unmask_interrupt(uint32_t reg);

void ioapic_mask_interrupt(uint32_t irq);
void ioapic_unmask_interrupt(uint32_t irq);

#ifdef __cplusplus
}
#endif

#endif /* APIC_H */


