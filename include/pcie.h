#ifndef PCIE_H
#define PCIE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCIE_MAX_DEVICES 256

typedef struct {
    uint8_t present;
    uint16_t segment;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision;
    uint8_t header_type;
    uint8_t multifunction;
} pcie_device_t;

typedef struct {
    uint64_t base_address;
    uint16_t segment_group;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
} MCFGEntry;

void pcie_init(void);

uint32_t pcie_get_device_count(void);

pcie_device_t* pcie_get_device(uint32_t index);

pcie_device_t* pcie_find_device(uint16_t vendor_id, uint16_t device_id);

pcie_device_t* pcie_find_by_class(uint8_t class_code, uint8_t subclass);

uint32_t pcie_read_bar(pcie_device_t *dev, uint8_t bar_index);

void pcie_write_bar(pcie_device_t *dev, uint8_t bar_index, uint32_t value);

void pcie_enable_bus_master(pcie_device_t *dev);

void pcie_enable_memory_space(pcie_device_t *dev);

void pcie_enable_io_space(pcie_device_t *dev);

void pcie_print_device(pcie_device_t *dev);

void pcie_list_devices(void);

#define PCI_CLASS_UNCLASSIFIED      0x00
#define PCI_CLASS_STORAGE           0x01
#define PCI_CLASS_NETWORK           0x02
#define PCI_CLASS_DISPLAY           0x03
#define PCI_CLASS_MULTIMEDIA        0x04
#define PCI_CLASS_MEMORY            0x05
#define PCI_CLASS_BRIDGE            0x06
#define PCI_CLASS_COMMUNICATION     0x07
#define PCI_CLASS_SYSTEM            0x08
#define PCI_CLASS_INPUT             0x09
#define PCI_CLASS_DOCKING           0x0A
#define PCI_CLASS_PROCESSOR         0x0B
#define PCI_CLASS_SERIAL            0x0C
#define PCI_CLASS_WIRELESS          0x0D
#define PCI_CLASS_INTELLIGENT       0x0E
#define PCI_CLASS_SATELLITE         0x0F
#define PCI_CLASS_ENCRYPTION        0x10
#define PCI_CLASS_SIGNAL_PROCESSING 0x11
#define PCI_CLASS_ACCELERATOR       0x12
#define PCI_CLASS_NON_ESSENTIAL     0x13
#define PCI_CLASS_COPROCESSOR       0x40
#define PCI_CLASS_UNASSIGNED        0xFF

#define PCI_SUBCLASS_IDE            0x01
#define PCI_SUBCLASS_SATA           0x06
#define PCI_SUBCLASS_NVME           0x08
#define PCI_SUBCLASS_ETHERNET       0x00
#define PCI_SUBCLASS_VGA_COMPATIBLE 0x00
#define PCI_SUBCLASS_XHCI           0x30

#ifdef __cplusplus
}
#endif

#endif
