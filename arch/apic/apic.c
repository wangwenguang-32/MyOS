#include<stdint.h>
#include<apic.h>
#include<io.h>
#include<cpu.h>
#include<addr_translation.h>


/* -------------------- MSR access -------------------- */
static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t lo = (uint32_t)val;
    uint32_t hi = (uint32_t)(val >> 32);
    __asm__ __volatile__("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}


int cpu_has_apic(void) {
    uint32_t a=1,b,c,d; cpuid_eax1(&a,&b,&c,&d);
    return (d & (1u<<9)) != 0;
}

static inline int cpu_has_x2apic(void) {
    uint32_t a=1,b,c,d; cpuid_eax1(&a,&b,&c,&d);
    return (c & (1u<<21)) != 0;
}

static void pic_disable(void) {
    outb(PIC1_DAT, 0xFF);
    outb(PIC2_DAT, 0xFF);
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);
    outb(PIC1_DAT, 0x20);
    outb(PIC2_DAT, 0x28);
    outb(PIC1_DAT, 0x04);
    outb(PIC2_DAT, 0x02);
    outb(PIC1_DAT, 0x01);
    outb(PIC2_DAT, 0x01);
    outb(PIC1_DAT, 0xFF);
    outb(PIC2_DAT, 0xFF);
}

static void lapic_enable(uint8_t spurious_vector) {
    uint64_t apic_base = rdmsr(IA32_APIC_BASE_MSR);
    apic_base |= ((uint64_t)IA32_APIC_BASE_ENABLE);
    if (cpu_has_x2apic()) {
        apic_base &= ~((uint64_t)IA32_X2APIC_ENABLE);
    }
    wrmsr(IA32_APIC_BASE_MSR, apic_base);

    const uint32_t SVR_ENABLE = 1u << 8;
    lapic_write(LAPIC_SVR, (uint32_t)spurious_vector | SVR_ENABLE);
    lapic_write(LAPIC_TPR, 0);

    const uint32_t LVT_MASKED = 1u << 16;
    lapic_write(LAPIC_LVT_LINT0, LVT_MASKED);
    lapic_write(LAPIC_LVT_LINT1, LVT_MASKED);
    lapic_write(LAPIC_LVT_PERF,  LVT_MASKED);
    lapic_write(LAPIC_LVT_THERMAL, LVT_MASKED);
    lapic_write(LAPIC_LVT_ERROR, LVT_MASKED);
    lapic_write(LAPIC_EOI, 0);
}

static uint32_t ioapic_get_gsi_count(void) {
    uint32_t ver = ioapic_read(IOAPIC_REG_VER);
    uint32_t max = (ver >> 16) & 0xFFu;
    return max + 1u;
}

static void ioapic_set_redirect(uint8_t irq, uint8_t vector, uint8_t dest_apic_id,
                                uint8_t level, uint8_t active_low, uint8_t masked) {
    uint32_t low = 0;
    if (active_low) low |= (1u << 13);
    if (level)      low |= (1u << 15);
    if (masked)     low |= (1u << 16);
    low |= vector; 
    uint32_t high = ((uint32_t)dest_apic_id) << 24;

    uint8_t reg = IOAPIC_REG_REDTBL + (uint8_t)(irq * 2);
    ioapic_write(reg,     low);
    ioapic_write(reg + 1, high);
}

static void ioapic_init_basic(uint8_t vector_base, uint8_t dest_apic_id) {
    uint32_t count = ioapic_get_gsi_count();
    uint32_t limit = (count < 24u) ? count : 24u;
    uint32_t irq = 0;
    for (irq ; irq < limit; ++irq) {
        ioapic_set_redirect((uint8_t)irq, (uint8_t)(vector_base + irq), dest_apic_id,0, 0, 1);
    }
}

/* -------------------- Public API -------------------- */
void apic_init(uint8_t spurious_vector, uint8_t ext_irq_vector_base) {
    map_virtual_to_physical(_pdt,0xFEE00000u,0xFEE00000u,3u);
    map_virtual_to_physical(_pdt,0xFEC00000u,0xFEC00000u,3u);
    if (!cpu_has_apic()) {
        printf("system has no apic");
        return;
    }
    pic_disable();
    lapic_enable(spurious_vector);
    uint8_t lapic_id = (uint8_t)(lapic_read(LAPIC_ID) >> 24);
    ioapic_init_basic(ext_irq_vector_base, lapic_id);
    
}

void lapic_eoi(void) {
    lapic_write(LAPIC_EOI, 0);
}

void lapic_mask_interrupt(uint32_t reg)
{
    uint32_t data=lapic_read(reg);
    data |= 1u << 16;
    lapic_write(reg,data);
}

void lapic_unmask_interrupt(uint32_t reg)
{
    uint32_t data=lapic_read(reg);
    data &= ~((uint32_t)1u << 16);
    lapic_write(reg,data);
}

void ioapic_mask_interrupt(uint32_t irq)
{
    uint8_t reg = IOAPIC_REG_REDTBL + (uint8_t)(irq * 2);
    uint32_t data=ioapic_read(reg);
    data |= 1u << 16;
    ioapic_write(reg,data);
}

void ioapic_unmask_interrupt(uint32_t irq)
{
    uint8_t reg = IOAPIC_REG_REDTBL + (uint8_t)(irq * 2);
    uint32_t data=ioapic_read(reg);
    data &= ~((uint32_t)1u << 16);
    ioapic_write(reg,data);
}
