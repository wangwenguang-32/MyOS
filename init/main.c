#include<printf.h>
#include<apic.h>
#include<keyboard.h>

void change_esp()
{
    asm("addl $0xC0000000,%esp;");
}


void main(unsigned long *esp)
{
    change_esp();
    cls();
    
    cmain(*(esp-2),*(esp-1));
    
    _init_mm();
    _init_paging();
     _init_idt();
     _apic_init(0xFF,0x20);
    keyboard_init();
    while(1);
}