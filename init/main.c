#include<printf.h>
#include<apic.h>
#include<keyboard.h>
#include<process.h>

void main(unsigned long *esp)
{
    asm("addl $0xC0000000,%esp;");
    cls();
    multiboot_info_process(*(esp-2),*(esp-1));
    init_idt();
    init_memory();
    apic_init(0xFF,0x20);
    keyboard_init();
    init_task();
    lapic_timer_init_periodic(0x20, 0x10000,128);
    move_to_user_mode();
    while(1);
}