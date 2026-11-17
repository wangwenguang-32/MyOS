#include<io.h>
#include<common.h>
#include<stdint.h>
#include<pci.h>

uint16_t pci_device_count;

uint32_t PCI_Read32(PCIAddress*addr,uint8_t off)
{
    uint8_t bus=addr->bus;
    uint8_t dev = addr->device;
    uint8_t func = addr->function;
    uint32_t address =
        (1<<31) |
        (bus<<16) |
        (dev<<11) |
        (func<<8) |
        (off & 0xfc);

    outl(PCI_ADDR,address);

    return inl(PCI_DATA);
}

uint32_t PCI_Read16(PCIAddress*addr,uint8_t off)
{
    uint8_t bus=addr->bus;
    uint8_t dev = addr->device;
    uint8_t func = addr->function;
    uint32_t address =
        (1<<31) |
        (bus<<16) |
        (dev<<11) |
        (func<<8) |
        (off & 0xfc);

    outl(PCI_ADDR,address);

    return inw(PCI_DATA);
}

uint32_t PCI_Read8(PCIAddress*addr,uint8_t off)
{
    uint8_t bus=addr->bus;
    uint8_t dev = addr->device;
    uint8_t func = addr->function;
    uint32_t address =
        (1<<31) |
        (bus<<16) |
        (dev<<11) |
        (func<<8) |
        (off & 0xfc);

    outl(PCI_ADDR,address);

    return inb(PCI_DATA);
}

void PCI_Write32(PCIAddress*addr,uint8_t off,uint32_t data)
{
    uint8_t bus=addr->bus;
    uint8_t dev = addr->device;
    uint8_t func = addr->function;
    uint32_t address =
        (1<<31) |
        (bus<<16) |
        (dev<<11) |
        (func<<8) |
        (off & 0xfc);

    outl(PCI_ADDR,address);
    outl(PCI_DATA,data);
}

void PCI_Write16(PCIAddress*addr,uint8_t off,uint32_t data)
{
    uint8_t bus=addr->bus;
    uint8_t dev = addr->device;
    uint8_t func = addr->function;
    uint32_t address =
        (1<<31) |
        (bus<<16) |
        (dev<<11) |
        (func<<8) |
        (off & 0xfc);

    outl(PCI_ADDR,address);
    outw(PCI_DATA,data);
}

void PCI_Write8(PCIAddress*addr,uint8_t off,uint32_t data)
{
    uint8_t bus=addr->bus;
    uint8_t dev = addr->device;
    uint8_t func = addr->function;
    uint32_t address =
        (1<<31) |
        (bus<<16) |
        (dev<<11) |
        (func<<8) |
        (off & 0xfc);

    outl(PCI_ADDR,address);
    outb(PCI_DATA,data);
}

bool PCI_ScanDevice(PCIScanState*state)
{
    PCIConfigSpace config;
    config.words[0]=PCI_Read32(&state->nextAddr,0);
    state->addr=state->nextAddr;
    state->nextAddr.function++;
    
    if(state->nextAddr.function==0x08)
    {
        state->nextAddr.function=0;
        state->nextAddr.device++;
        if(state->nextAddr.device==0x20)
        {
            state->nextAddr.device=0;
            state->nextAddr.bus++;
            if(state->nextAddr.bus==PCI_MAX_BUS)
            {
                return false;
            }
        }
    }
    
    if(config.words[0]!=0xFFFFFFFFUL)
    {
        state->vendorId=config.vendorId;
        state->deviceId=config.deviceId;
        return true;
    }
    

}



bool PCI_FindDevice(uint16_t vendorId,uint16_t deviceId,PCIAddress*addrOut)
{
    PCIScanState state;
    state.nextAddr.bus=0;
    state.nextAddr.device=0;
    state.nextAddr.function=0;
    while(PCI_ScanDevice(&state))
    {
        if(state.vendorId==vendorId&&state.deviceId==deviceId)
        {
            *addrOut=state.addr;
            return true;
        }
    }
    return false;

}

uint32_t PCI_GetBarAddr(PCIAddress*addr,uint32_t index)
{
    uint32_t bar = PCI_Read32(addr,offsetof(PCIConfigSpace,BAR[index]));
    if (bar & 1) 
    {
    bar = bar & 0xFFFFFFFC;  // 屏蔽低2位
    } 
    else 
    {
    bar= bar & 0xFFFFFFF0; // 屏蔽低4位
    }
    return bar;
}

uint32_t PCI_GetBarSize(PCIAddress* addr, uint32_t index)
{

    uint32_t old = PCI_Read32(addr, offsetof(PCIConfigSpace,BAR[index]));

    PCI_Write32(addr, offsetof(PCIConfigSpace,BAR[index]), 0xFFFFFFFF);

    uint32_t mask = PCI_Read32(addr, offsetof(PCIConfigSpace,BAR[index]));

    PCI_Write32(addr, offsetof(PCIConfigSpace,BAR[index]), old);

    if( (old & 1) == 0 )
        return (~(mask & 0xFFFFFFF0)) + 1;
    else
        return (~(mask & 0xFFFFFFFC)) + 1;
}

void PCI_SetCommand(PCIAddress*addr,uint16_t command)
{
    PCI_Write16(addr,offsetof(PCIConfigSpace,command),command);
}

uint16_t PCI_GetCommand(PCIAddress*addr)
{
    return PCI_Read16(addr,offsetof(PCIConfigSpace,command));
}