#ifndef SVGA_H
#define SVGA_H

#include<pci.h>
#include<stdint.h>
#include<common.h>
#include<svga_registers.h>

typedef struct SVGADevice {
   PCIAddress pciAddr;
   uint32_t     ioBase;
   uint32_t*    fifoMem;
   uint32_t*    pfifoMem;
   uint8_t*     fbMem;
   uint8_t*     pfbMem;
   uint32_t     fifoSize;
   uint32_t     fbSize;
   uint32_t     vramSize;

   uint32_t     deviceVersionId;
   uint32_t     capabilities;

   uint32_t     width;
   uint32_t     height;
   uint32_t     bpp;
   uint32_t     pitch;

   struct {
      uint32_t  reservedSize;
      bool    usingBounceBuffer;
      uint8_t   bounceBuffer[1024 * 1024];
      uint32_t  nextFence;
   } fifo;

   volatile struct {
      uint32_t        pending;
      uint32_t        switchContext;
    //   IntrContext   oldContext;
    //   IntrContext   newContext;
      uint32_t        count;
   } irq;

} SVGADevice;

#define PCI_VENDOR_ID_VMWARE  0x15AD
#define PCI_DEVICE_ID_SVGA2   0x0405

#endif