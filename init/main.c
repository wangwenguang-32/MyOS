#include<printf.h>
#include<apic.h>
#include<keyboard.h>
#include<process.h>
#include <memory.h>

void main(unsigned long *esp)
{
    asm("addl $0xC0000000,%esp;");
    cls();
    multiboot_info_process(*(esp-2),*(esp-1));
    
    init_idt();
    int memory_error = init_memory();
    if (memory_error) {
        printf("Memory initialization failed: %d\n", memory_error);
        for (;;) __asm__ volatile("cli; hlt");
    }
    apic_init(0xFF,0x20);
    keyboard_init();
    // svga_init();
    // blk_init();
    // mbr_init();
    // tty_init();
    while(1);
    init_task();
    lapic_timer_init_periodic(0x20, 0x10000,128);
    move_to_user_mode();
}