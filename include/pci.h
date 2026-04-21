#ifndef PCI_H
#define PCI_H

#include<stdint.h>
#include<common.h>

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC
#define PCI_MAX_BUS 0x1


// PCI Command 寄存器 位定义
#define PCI_CMD_IO_SPACE        (1 << 0)   // 0: IO 空间使能
#define PCI_CMD_MEM_SPACE       (1 << 1)   // 1: MMIO 内存空间使能
#define PCI_CMD_BUS_MASTER      (1 << 2)   // 2: 总线主控（DMA 使能）
#define PCI_CMD_SPECIAL_CYCLE   (1 << 3)   // 3: 特殊周期（几乎不用）
#define PCI_CMD_MEM_WI_INV      (1 << 4)   // 4: 内存写并刷新
#define PCI_CMD_VGA_PALETTE     (1 << 5)   // 5: VGA 调色板侦测
#define PCI_CMD_PARITY_ERR      (1 << 6)   // 6: 奇偶校验错误响应
#define PCI_CMD_SERR_ENABLE     (1 << 8)   // 8: SERR 系统错误使能
#define PCI_CMD_INT_DISABLE     (1 << 10)  // 10: 中断禁用（1=禁用）

typedef union PCIConfigSpace {
   uint32_t words[16];
   struct {
      uint16_t vendorId;
      uint16_t deviceId;
      uint16_t command;
      uint16_t status;
      uint16_t revisionId;
      uint8_t  subclass;
      uint8_t  classCode;
      uint8_t  cacheLineSize;
      uint8_t  latTimer;
      uint8_t  headerType;
      uint8_t  BIST;
      uint32_t BAR[6];
      uint32_t cardbusCIS;
      uint16_t subsysVendorId;
      uint16_t subsysId;
      uint32_t expansionRomAddr;
      uint32_t reserved0;
      uint32_t reserved1;
      uint8_t  intrLine;
      uint8_t  intrPin;
      uint8_t  minGrant;
      uint8_t  maxLatency;
   };
} __attribute__ ((__packed__)) PCIConfigSpace;

typedef struct PCIAddress {
   uint8_t bus, device, function;
} PCIAddress;

typedef struct PCIScanState {
   uint16_t     vendorId;
   uint16_t     deviceId;
   PCIAddress nextAddr;
   PCIAddress addr;
} PCIScanState;

bool PCI_ScanDevice(PCIScanState*state);
bool PCI_FindDevice(uint16_t vendorId,uint16_t deviceId,PCIAddress*addrOut);
uint32_t PCI_GetBarAddr(PCIAddress*addr,uint32_t index);
uint32_t PCI_GetBarSize(PCIAddress* addr, uint32_t index);
void PCI_SetCommand(PCIAddress*addr,uint16_t command);
uint16_t PCI_GetCommand(PCIAddress*addr);

#endif