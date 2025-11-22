#include<printf.h>
#include<process.h>
#include<stdint.h>
#include<addr_translation.h>

#define PF_ERR_PRESENT       (1 << 0)  // Bit0：页存在位（0=页不存在，1=页存在但权限错误）
#define PF_ERR_WRITE         (1 << 1)  // Bit1：写操作位（0=读/执行操作，1=写操作）
#define PF_ERR_USER          (1 << 2)  // Bit2：用户态位（0=内核态访问，1=用户态访问）
#define PF_ERR_RESERVED      (1 << 3)  // Bit3：保留位违规（0=无，1=访问了保留位地址）
#define PF_ERR_INSTRUCTION   (1 << 4)  // Bit4：指令获取位（0=数据访问，1=取指令时异常）


void handle_user_write_prot_fault(unsigned long fault_addr) 
{
    

}



void do_page_fault(uint32_t error_code)
{
    while(1);
    uint32_t addr=0;
    __asm__("movl %%cr2,%%eax;":"=a"(addr) :);
    if(error_code==(PF_ERR_USER | PF_ERR_WRITE | PF_ERR_PRESENT))
    {
        handle_user_write_prot_fault(addr);
    }
    
}