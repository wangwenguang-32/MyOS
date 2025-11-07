#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ACPI structures are packed */
#pragma pack(push, 1)

typedef struct {
    char     signature[8];          /* "RSD PTR " */
    uint8_t  checksum;              /* v1 checksum over first 20 bytes */
    char     oem_id[6];
    uint8_t  revision;              /* 0=ACPI 1.0, >=2 for ACPI 2.0+ */
    uint32_t rsdt_physical_address; /* RSDT (32-bit) */
    uint32_t length;                /* total length of RSDP (>= ACPI 2.0) */
    uint64_t xsdt_physical_address; /* XSDT (64-bit) */
    uint8_t  extended_checksum;     /* checksum over entire table */
    uint8_t  reserved[3];
} RSDPDescriptor20;

typedef struct {
    char     signature[4];
    uint32_t length;                /* total length */
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} ACPISDTHeader;

/* MADT (APIC) */
typedef struct {
    ACPISDTHeader header;           /* signature = "APIC" */
    uint32_t local_apic_address;    /* LAPIC MMIO base */
    uint32_t flags;                 /* bit0: PC-AT compat */
    /* followed by variable entries */
} MADT;

typedef struct {
    uint8_t type;
    uint8_t length;
} MADTEntryHeader;

enum {
    MADT_LAPIC            = 0,
    MADT_IOAPIC           = 1,
    MADT_INT_SRC_OVERRIDE = 2,
    MADT_NMI_SRC          = 3,
    MADT_LAPIC_NMI        = 4,
    MADT_LAPIC_ADDR_OVERRIDE = 5,
};

typedef struct { /* type 0 */
    MADTEntryHeader h;
    uint8_t  acpi_processor_id;
    uint8_t  apic_id;
    uint32_t flags;
} MADTLocalApic;

typedef struct { /* type 1 */
    MADTEntryHeader h;
    uint8_t  ioapic_id;
    uint8_t  reserved;
    uint32_t ioapic_address;
    uint32_t global_system_interrupt_base;
} MADTIOApic;

typedef struct { /* type 2 */
    MADTEntryHeader h;
    uint8_t  bus_source;            /* should be 0 for ISA */
    uint8_t  irq_source;            /* ISA IRQ */
    uint32_t global_system_interrupt;
    uint16_t flags;                 /* polarity/trigger */
} MADTIntSrcOverride;

#pragma pack(pop)

/* Public API */

/* Initialize ACPI by scanning for RSDP and validating tables. Returns non-zero on success. */
int acpi_init(void);

/* Get mapped pointers to RSDT/XSDT headers if present (may be NULL) */
const ACPISDTHeader* acpi_get_rsdt(void);
const ACPISDTHeader* acpi_get_xsdt(void);

/* Find a table by 4-char signature, prefer XSDT if available; returns header or NULL. */
const ACPISDTHeader* acpi_find_table(const char sig[4]);

/* MADT helpers: return first IOAPIC / LAPIC base found and counts; may return 0 if not found */
uint32_t acpi_get_lapic_base(void);
uint32_t acpi_get_ioapic_base(uint32_t *gsi_base_out);

#ifdef __cplusplus
}
#endif

#endif /* ACPI_H */


