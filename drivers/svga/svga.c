#include<svga.h>
#include<stdint.h>
#include<pci.h>
#include<printf.h>
#include<io.h>
#include<common.h>

SVGADevice svga;

// 写 SVGA 寄存器
void SVGA_WriteReg(uint16_t bar0, uint32_t index, uint32_t value) {
    outl(bar0 + 0x00, index);
    outl(bar0 + 0x01, value);
}

// 读 SVGA 寄存器
uint32_t SVGA_ReadReg(uint16_t bar0, uint32_t index) {
    outl(bar0 + 0x00, index);
    return inl(bar0 + 0x01);
}

bool svga_set_mode(uint32_t width,uint32_t height)
{
    uint32_t max_width=SVGA_ReadReg(svga.ioBase,SVGA_REG_MAX_WIDTH);
    uint32_t max_height=SVGA_ReadReg(svga.ioBase,SVGA_REG_MAX_HEIGHT);

    if(width>max_width||height>max_height)
    {
        return false;
    }

    SVGA_WriteReg(svga.ioBase,SVGA_REG_WIDTH,width);
    SVGA_WriteReg(svga.ioBase,SVGA_REG_HEIGHT,height);
}


void svga_enable()
{
    svga.fifoMem[SVGA_FIFO_MIN]=SVGA_FIFO_NUM_REGS*sizeof(uint32_t);
    svga.fifoMem[SVGA_FIFO_MAX]=(uint32_t)svga.fifoMem+svga.fifoSize-1;
    svga.fifoMem[SVGA_FIFO_NEXT_CMD]=svga.fifoMem[SVGA_FIFO_MIN];
    svga.fifoMem[SVGA_FIFO_STOP]=svga.fifoMem[SVGA_FIFO_MIN];
    svga_set_mode(800,600);
    SVGA_WriteReg(svga.ioBase,SVGA_REG_ENABLE,SVGA_REG_ENABLE_ENABLE);
}



void svga_init()
{
    if(!PCI_FindDevice(PCI_VENDOR_ID_VMWARE,PCI_DEVICE_ID_SVGA2,&svga.pciAddr))
    {
        printf("No SVGA\n");
        return;
    }
    svga.ioBase = PCI_GetBarAddr(&svga.pciAddr,0);
    svga.fbMem = PCI_GetBarAddr(&svga.pciAddr,1);
    svga.fifoMem = PCI_GetBarAddr(&svga.pciAddr,2);
    svga.vramSize = SVGA_ReadReg(svga.ioBase,SVGA_REG_VRAM_SIZE);
    svga.fbSize = SVGA_ReadReg(svga.ioBase,SVGA_REG_FB_SIZE);
    svga.fifoSize = SVGA_ReadReg(svga.ioBase,SVGA_REG_MEM_SIZE);
    svga.deviceVersionId = SVGA_ReadReg(svga.ioBase,SVGA_REG_ID);
    if(svga.deviceVersionId!=SVGA_ID_2)
    {
        return;
    }
    
}