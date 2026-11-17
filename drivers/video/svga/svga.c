#include<svga.h>
#include<stdint.h>
#include<pci.h>
#include<printf.h>
#include<io.h>
#include<common.h>
#include<io_mapping.h>
#include<paging.h>

SVGADevice svga;

// 写 SVGA 寄存器
void svga_write_reg(uint32_t bar0, uint32_t index, uint32_t value) {
    outl(bar0 + SVGA_INDEX_PORT, index);
    outl(bar0 + SVGA_VALUE_PORT, value);
}

// 读 SVGA 寄存器
uint32_t svga_read_reg(uint32_t bar0, uint32_t index) {
    outl(bar0 + SVGA_INDEX_PORT, index);
    return inl(bar0 + SVGA_VALUE_PORT);
}

bool svga_set_mode(uint32_t width,uint32_t height)
{
    svga.width=width;
    svga.height=height;
    
    uint32_t max_width=svga_read_reg(svga.ioBase,SVGA_REG_MAX_WIDTH);
    uint32_t max_height=svga_read_reg(svga.ioBase,SVGA_REG_MAX_HEIGHT);

    if(width>max_width||height>max_height)
    {
        return false;
    }

    svga_write_reg(svga.ioBase,SVGA_REG_WIDTH,width);
    svga_write_reg(svga.ioBase,SVGA_REG_HEIGHT,height);
    svga_write_reg(svga.ioBase,SVGA_REG_ENABLE,SVGA_REG_ENABLE_ENABLE);
    svga.bpp = svga_read_reg(svga.ioBase,SVGA_REG_BITS_PER_PIXEL);
    svga.pitch = svga_read_reg(svga.ioBase,SVGA_REG_BYTES_PER_LINE);
    return svga.bpp == 32 && svga.pitch != 0;
}

void svga_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color)
{
    if (svga.fbMem == NULL || svga.pitch == 0 || svga.bpp != 32) {
        return;
    }

    if (x >= svga.width || y >= svga.height) {
        return;
    }

    if (x + width > svga.width) {
        width = svga.width - x;
    }

    if (y + height > svga.height) {
        height = svga.height - y;
    }

    uint32_t bytes_per_pixel = svga.bpp >> 3;
    for (uint32_t row = 0; row < height; ++row) {
        uint32_t *pixel = (uint32_t *)(svga.fbMem + (y + row) * svga.pitch + x * bytes_per_pixel);
        for (uint32_t col = 0; col < width; ++col) {
            pixel[col] = color;
        }
    }
}


void svga_enable()
{
    
    svga.fifoMem[SVGA_FIFO_MIN]=SVGA_FIFO_NUM_REGS*sizeof(uint32_t);
    svga.fifoMem[SVGA_FIFO_MAX]=svga.fifoSize;
    svga.fifoMem[SVGA_FIFO_NEXT_CMD]=svga.fifoMem[SVGA_FIFO_MIN];
    svga.fifoMem[SVGA_FIFO_STOP]=svga.fifoMem[SVGA_FIFO_MIN];
    svga_write_reg(svga.ioBase,SVGA_REG_ENABLE,SVGA_REG_ENABLE_ENABLE);
    svga_write_reg(svga.ioBase,SVGA_REG_CONFIG_DONE,1);
}

bool svga_has_fifo_cap(uint32_t cap)
{
   if((svga.fifoMem[SVGA_FIFO_CAPABILITIES] & cap)!=0)
    {
      return true;
    }
    return false;
}

void *
svga_fifo_reserve(uint32_t bytes)  // IN
{
   volatile uint32_t *fifo = svga.fifoMem;
   uint32_t max = fifo[SVGA_FIFO_MAX];
   uint32_t min = fifo[SVGA_FIFO_MIN];
   uint32_t nextCmd = fifo[SVGA_FIFO_NEXT_CMD];
   bool reserveable = svga_has_fifo_cap(SVGA_FIFO_CAP_RESERVE);
   // if(!reserveable)
   // {
   //    while(1);
   // }
   if (bytes > sizeof svga.fifo.bounceBuffer ||
       bytes > (max - min)) {
      return NULL;
   }

   if (bytes % sizeof(uint32_t)) {
      return NULL;
   }

   if (svga.fifo.reservedSize != 0) {
      return NULL;
   }

   svga.fifo.reservedSize = bytes;

   while (1) 
   {
      uint32_t stop = fifo[SVGA_FIFO_STOP];
      bool reserveInPlace = false;
      bool needBounce = false;


      if (nextCmd >= stop) {
         /* There is no valid FIFO data between nextCmd and max */

         if (nextCmd + bytes < max ||
             (nextCmd + bytes == max && stop > min)) {
            
            reserveInPlace = true;
         } else if ((max - nextCmd) + (stop - min) <= bytes) {
            
            // SVGAFIFOFull();
         } else {
           
            needBounce = true;
         }

      } else {
         /* There is FIFO data between nextCmd and max */

         if (nextCmd + bytes < stop) {
            /*
             * Fastest path 2: There is already enough contiguous space
             * between nextCmd and stop.
             */
            reserveInPlace = true;
         } else {
            /*
             * There isn't enough room between nextCmd and stop.
             * The FIFO is too full to accept this command.
             */
            // SVGAFIFOFull();
         }
      }

      /*
       * If we decided we can write directly to the FIFO, make sure
       * the VMX can safely support this.
       */
      if (reserveInPlace) 
	  {
         
         if (reserveable || bytes <= sizeof(uint32_t)) 
		   {
            
            svga.fifo.usingBounceBuffer = false;
            if (reserveable) 
			   {
               fifo[SVGA_FIFO_RESERVED] = bytes;
            }
            
            return nextCmd + (uint8_t*) fifo;
         } 
		   else 
		   {
            /*
             * Need to bounce because we can't trust the VMX to safely
             * handle uncommitted data in FIFO.
             */
            needBounce = true;
         }
      }

      /*
       * If we reach here, either we found a full FIFO, called
       * SVGAFIFOFull to make more room, and want to try again, or we
       * decided to use a bounce buffer instead.
       */
      if (needBounce) {
         svga.fifo.usingBounceBuffer = true;
         return svga.fifo.bounceBuffer;
      }
   } /* while (1) */
}




void
svga_fifo_commit(uint32_t bytes)  // IN
{
   volatile uint32_t *fifo = svga.fifoMem;
   uint32_t nextCmd = fifo[SVGA_FIFO_NEXT_CMD];
   uint32_t max = fifo[SVGA_FIFO_MAX];
   uint32_t min = fifo[SVGA_FIFO_MIN];
   bool reserveable = svga_has_fifo_cap(SVGA_FIFO_CAP_RESERVE);
   
   if (svga.fifo.reservedSize == 0) {
      return;
   }
   svga.fifo.reservedSize = 0;

   if (svga.fifo.usingBounceBuffer) 
   {
      /*
       * Slow paths: copy out of a bounce buffer.
       */
      uint8_t *buffer = svga.fifo.bounceBuffer;

      if (reserveable) 
	  {
        

         uint32_t chunkSize = MIN(bytes, max - nextCmd);
         fifo[SVGA_FIFO_RESERVED] = bytes;
         memcpy(nextCmd + (uint8_t*) fifo, buffer, chunkSize);
         memcpy(min + (uint8_t*) fifo, buffer + chunkSize, bytes - chunkSize);

      } 
	  else 
	  {
         uint32_t *dword = (uint32_t *)buffer;

         while (bytes > 0) 
         {
            
            fifo[nextCmd / 4] = *dword++;
            nextCmd += 4;
            if (nextCmd == max) 
            {
               nextCmd = min;
            }
           
            
            fifo[SVGA_FIFO_NEXT_CMD] = nextCmd;
            bytes -= 4;
            
         }
         
      }
   }
  

   /*
    * Atomically update NEXT_CMD, if we didn't already
    */
   if (!svga.fifo.usingBounceBuffer || reserveable) 
   {
      nextCmd += bytes;
      if (nextCmd >= max) 
      {
         nextCmd -= max - min;
      }
      fifo[SVGA_FIFO_NEXT_CMD] = nextCmd;
   }

   /*
    * Clear the reservation in the FIFO.
    */
   if (reserveable) 
   {
      fifo[SVGA_FIFO_RESERVED] = 0;
   }
}



void svga_fifo_commit_all(void)
{
   svga_fifo_commit(svga.fifo.reservedSize);
   svga_write_reg(svga.ioBase, SVGA_REG_SYNC, 1);
   while (svga_read_reg(svga.ioBase, SVGA_REG_BUSY)) {
   }
}


void *
svga_fifo_reserve_cmd(uint32_t type,   // IN
                    uint32_t bytes)  // IN
{
   uint32_t *cmd = svga_fifo_reserve(bytes + 4);
   if (cmd == NULL) {
      return NULL;
   }
   cmd[0] = type;
   return cmd + 1;
}

void svga_fifo_update(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    SVGAFifoCmdUpdate *cmd = (SVGAFifoCmdUpdate *)svga_fifo_reserve_cmd(SVGA_CMD_UPDATE, sizeof(SVGAFifoCmdUpdate));
    if (cmd == NULL) {
        return;
    }

    cmd->x = x;
    cmd->y = y;
    cmd->width = width;
    cmd->height = height;
    svga_fifo_commit_all();
}

void svga_init()
{
    if(!PCI_FindDevice(PCI_VENDOR_ID_VMWARE,PCI_DEVICE_ID_SVGA2,&svga.pciAddr))
    {
        //printf("No SVGA\n");
        return;
    }

    uint16_t pci_command = PCI_GetCommand(&svga.pciAddr);
    pci_command |= PCI_CMD_IO_SPACE | PCI_CMD_MEM_SPACE | PCI_CMD_BUS_MASTER;
    PCI_SetCommand(&svga.pciAddr, pci_command);

    svga.ioBase = PCI_GetBarAddr(&svga.pciAddr,0);
    svga.pfbMem = (uint8_t *)PCI_GetBarAddr(&svga.pciAddr,1);
    svga.pfifoMem = (uint32_t *)PCI_GetBarAddr(&svga.pciAddr,2);
    svga.vramSize = svga_read_reg(svga.ioBase,SVGA_REG_VRAM_SIZE);
    svga.fbSize = PCI_GetBarSize(&svga.pciAddr,1);
    svga.fifoSize = PCI_GetBarSize(&svga.pciAddr,2);
    svga.fbMem = (uint8_t*)iomap(svga.pfbMem,svga.fbSize);
    svga.fifoMem = (uint32_t*)iomap(svga.pfifoMem,svga.fifoSize);
    svga_write_reg(svga.ioBase, SVGA_REG_ID, SVGA_ID_2);
    svga.deviceVersionId = svga_read_reg(svga.ioBase,SVGA_REG_ID);
    if(svga.deviceVersionId!=SVGA_ID_2)
    {
        printf("SVGA version mismatch: %x\n", svga.deviceVersionId);
        return;
    }
    
   //  map_virtual_range_to_physical(_pdt,(uint32_t)svga.fifoMem,(uint32_t)svga.fifoMem,(svga.fifoSize + 0xFFF) >> 12,PAGE_PRESENT|PAGE_WRITE);
   //  map_virtual_range_to_physical(_pdt,(uint32_t)svga.fbMem,(uint32_t)svga.fbMem,(svga.fbSize + 0xFFF) >> 12,PAGE_PRESENT|PAGE_WRITE);

    svga_enable();
    if(!svga_set_mode(1000,800))
    {
        printf("SVGA mode set failed\n");
        return;
    }

    memzero(svga.fbMem,svga.fbSize);
    svga_fill_rect(80, 60, 320, 180, 0x00FF0000);
    svga_fifo_update(80, 60, 320, 180);
}
