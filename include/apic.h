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

#ifdef __cplusplus
}
#endif

#endif /* APIC_H */


