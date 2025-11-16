#include<printf.h>
#include<apic.h>
#include<keyboard.h>
#include<process.h>

void change_esp()
{
    asm("addl $0xC0000000,%esp;");
}

void main(unsigned long *esp)
{
    change_esp();
    cls();
    cmain(*(esp-2),*(esp-1));
    _init_idt();
    _init_mm();
    _init_paging();
    _apic_init(0xFF,0x20);
    _keyboard_init();
    _init_task0();
    lapic_timer_init_periodic(0x20, 0x10000,128);
    move_to_user_mode();
    while(1);
}