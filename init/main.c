#include <stdint.h>
#include <printf.h>
#include <apic.h>
#include <keyboard.h>
#include <tty.h>
#include <memory.h>
#include <multiboot2.h>
#include <direct_map.h>
#include <interrupts.h>

extern void multiboot_info_process(unsigned long magic, unsigned long address);

static void boot_failure(const char *stage)
{
    printf("Boot failed: %s\n", stage);
    for (;;) __asm__ volatile("cli; hlt" ::: "memory");
}

void main(uint32_t magic, uint32_t boot_info_physical)
{
    __asm__ volatile("cli" ::: "memory");
    cls();
    init_idt();
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC || (boot_info_physical & 7U) ||
        !direct_map_contains_physical(boot_info_physical, 8))
        boot_failure("invalid Multiboot2 information");
    uint32_t *info = direct_map_pointer(boot_info_physical);
    if (info[0] < 16 || !direct_map_contains_physical(boot_info_physical, info[0]))
        boot_failure("Multiboot2 information outside bootstrap mapping");
    /* Pass the physical resource address; the parser uses its high alias. */
    multiboot_info_process(magic, boot_info_physical);
    int result = init_memory();
    if (result) {
        printf("Memory initialization error: %d\n", result);
        boot_failure("memory initialization");
    }
    tty_init();
    apic_init(0xFF, 0x20);
    keyboard_init();
    printf("Kernel initialized; waiting for interrupts.\n");
    /* Temporary boot idle loop. No task exists yet; do not start scheduling
     * until task0 has a dedicated idle stack and a valid initial context. */
    for (;;) __asm__ volatile("sti; hlt" ::: "memory");
}
