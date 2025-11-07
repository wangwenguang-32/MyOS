#include <stdint.h>
#include "acpi.h"
#include <printf.h>
/*
 * Minimal ACPI parsing for 32-bit freestanding OS.
 * - RSDP scan: EBDA (0x40E:EBDA seg) 1KB + BIOS area 0xE0000..0xFFFFF
 * - Validate checksum
 * - Load RSDT/XSDT
 * - Find MADT (APIC) and expose LAPIC/IOAPIC info
 * Assumes identity mapping for physical addresses in early boot.
 */

#define BIOS_ROM_START   0xE0000u
#define BIOS_ROM_END     0x100000u

/* BDA: EBDA segment stored at 0x40E (word, segment) */
static inline uint16_t bda_read16(uint32_t offset) {
    volatile const uint8_t *bda = (const uint8_t *)(uintptr_t)0x400;
    uint16_t lo = bda[offset];
    uint16_t hi = bda[offset+1];
    return (uint16_t)(lo | (hi<<8));
}

static inline uint8_t sum_bytes(const void *p, uint32_t len) {
    const uint8_t *b = (const uint8_t *)p;
    uint32_t s = 0;
    int32_t i=0;
    for (i;i<len;i++) s += b[i];
    return (uint8_t)(s & 0xFF);
}

static const RSDPDescriptor20 *g_rsdp;
static const ACPISDTHeader *g_rsdt;
static const ACPISDTHeader *g_xsdt;

static const RSDPDescriptor20* scan_rsdp_range(uintptr_t start, uintptr_t end) {
    uintptr_t p = start;
    for (p; p + sizeof(RSDPDescriptor20) <= end; p += 16) {
        const RSDPDescriptor20 *r = (const RSDPDescriptor20 *)p;
        if (r->signature[0]=='R' && r->signature[1]=='S' && r->signature[2]=='D' && r->signature[3]==' ' &&
            r->signature[4]=='P' && r->signature[5]=='T' && r->signature[6]=='R' && r->signature[7]==' ') {
            /* v1 checksum over first 20 bytes */
            if (sum_bytes(r, 20) == 0) {
                if (r->revision >= 2) {
                    if (sum_bytes(r, r->length) != 0) {
                        continue;
                    }
                }
                return r;
            }
        }
    }
    return 0;
}

static const RSDPDescriptor20* find_rsdp(void) {
    /* EBDA */
    uint16_t ebda_seg = bda_read16(0x0E);
    uintptr_t ebda = ((uintptr_t)ebda_seg) << 4;
    if (ebda >= 0x80000 && ebda < 0xA0000) {
        const RSDPDescriptor20 *r = scan_rsdp_range(ebda, ebda + 1024);
        if (r) return r;
    }
    /* BIOS ROM area */
    return scan_rsdp_range(BIOS_ROM_START, BIOS_ROM_END);
}

static int validate_sdt(const ACPISDTHeader *h) {
    if (!h) return 0;
    return sum_bytes(h, h->length) == 0;
}

static const ACPISDTHeader* find_in_rsdt(const ACPISDTHeader *rsdt, const char sig[4]) {
    if (!rsdt) return 0;
    if (!validate_sdt(rsdt)) return 0;
    uint32_t entries = (rsdt->length - sizeof(ACPISDTHeader)) / 4;
    const uint32_t *ptrs = (const uint32_t *)(rsdt + 1);
    uint32_t i=0;
    for (i;i<entries;i++) {
        const ACPISDTHeader *t = (const ACPISDTHeader *)(uintptr_t)ptrs[i];
        if (!t) continue;
        if (t->signature[0]==sig[0] && t->signature[1]==sig[1] && t->signature[2]==sig[2] && t->signature[3]==sig[3]) {
            if (validate_sdt(t)) return t;
        }
    }
    return 0;
}

static const ACPISDTHeader* find_in_xsdt(const ACPISDTHeader *xsdt, const char sig[4]) {
    if (!xsdt) return 0;
    if (!validate_sdt(xsdt)) return 0;
    uint32_t entries = (xsdt->length - sizeof(ACPISDTHeader)) / 8;
    const uint64_t *ptrs = (const uint64_t *)(xsdt + 1);
    uint32_t i=0;
    for (i;i<entries;i++) {
        const ACPISDTHeader *t = (const ACPISDTHeader *)(uintptr_t)ptrs[i];
        if (!t) continue;
        if (t->signature[0]==sig[0] && t->signature[1]==sig[1] && t->signature[2]==sig[2] && t->signature[3]==sig[3]) {
            if (validate_sdt(t)) return t;
        }
    }
    return 0;
}

int acpi_init(void) {
    g_rsdp = find_rsdp();
    if (!g_rsdp) return 0;
    g_rsdt = 0; g_xsdt = 0;
    if (g_rsdp->rsdt_physical_address) {
        g_rsdt = (const ACPISDTHeader *)(uintptr_t)g_rsdp->rsdt_physical_address;
        if (!validate_sdt(g_rsdt)) g_rsdt = 0;
    }
    if (g_rsdp->revision >= 2 && g_rsdp->xsdt_physical_address) {
        g_xsdt = (const ACPISDTHeader *)(uintptr_t)g_rsdp->xsdt_physical_address;
        if (!validate_sdt(g_xsdt)) g_xsdt = 0;
    }
    return (g_rsdt || g_xsdt) ? 1 : 0;
}

const ACPISDTHeader* acpi_get_rsdt(void) { return g_rsdt; }
const ACPISDTHeader* acpi_get_xsdt(void) { return g_xsdt; }

const ACPISDTHeader* acpi_find_table(const char sig[4]) {
    const ACPISDTHeader *t = 0;
    if (g_xsdt) t = find_in_xsdt(g_xsdt, sig);
    if (!t && g_rsdt) t = find_in_rsdt(g_rsdt, sig);
    return t;
}

uint32_t acpi_get_lapic_base(void) {
    const ACPISDTHeader *h = acpi_find_table("APIC");
    if (!h) return 0;
    const MADT *m = (const MADT *)h;
    if (!validate_sdt(&m->header)) return 0;
    /* LAPIC base may be overridden by type 5 entry; walk entries */
    uint32_t lapic = m->local_apic_address;
    const uint8_t *p = (const uint8_t *)(m + 1);
    const uint8_t *end = ((const uint8_t*)m) + m->header.length;
    while (p + sizeof(MADTEntryHeader) <= end) {
        const MADTEntryHeader *eh = (const MADTEntryHeader *)p;
        if (eh->length == 0) break;
        if (eh->type == MADT_LAPIC_ADDR_OVERRIDE && eh->length >= 12) {
            /* struct: type(1),len(1),addr(8) */
            const uint32_t *low = (const uint32_t *)(p + 4);
            lapic = *low; /* low 32 bits, 32-bit OS path */
        }
        p += eh->length;
    }
    return lapic;
}

uint32_t acpi_get_ioapic_base(uint32_t *gsi_base_out) {
    const ACPISDTHeader *h = acpi_find_table("APIC");
    if (!h) return 0;
    const MADT *m = (const MADT *)h;
    if (!validate_sdt(&m->header)) return 0;
    const uint8_t *p = (const uint8_t *)(m + 1);
    const uint8_t *end = ((const uint8_t*)m) + m->header.length;
    while (p + sizeof(MADTEntryHeader) <= end) {
        const MADTEntryHeader *eh = (const MADTEntryHeader *)p;
        if (eh->length == 0) break;
        if (eh->type == MADT_IOAPIC && eh->length >= sizeof(MADTIOApic)) {
            const MADTIOApic *io = (const MADTIOApic *)p;
            if (gsi_base_out) *gsi_base_out = io->global_system_interrupt_base;
            return io->ioapic_address;
        }
        p += eh->length;
    }
    return 0;
}



