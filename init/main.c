#include <stdint.h>
#include <printf.h>
#include <apic.h>
#include <keyboard.h>
#include <tty.h>
#include <memory.h>
#include <multiboot2.h>
#include <direct_map.h>
#include <interrupts.h>
#include <boot_info.h>
#include <physical_memory.h>
#include <kernel.h>
#include <process.h>
#include <scheduler.h>



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
    if (multiboot_info_process(magic, boot_info_physical))
        boot_failure("malformed or missing boot memory information");
    int result = init_memory();
    if (result) {
        printf("Memory initialization error: %d\n", result);
        boot_failure("memory initialization");
    }
    physical_memory_stats_t stats;
    physical_memory_get_stats(&stats);
    printf("[memory] Managed=%u KiB allocated=%u KiB free=%u KiB\n",
           stats.managed_pages * 4, stats.allocated_pages * 4, stats.free_pages * 4);
    printf("[memory] Direct map: 0x%x..0x%x\n",
           (uint32_t)DIRECT_MAP_START, (uint32_t)DIRECT_MAP_END);
    printf("[memory] Heap ready (pages allocated on demand).\n");
    tty_init();
    apic_init(0xFF, 0x20);
    keyboard_init();
    if (init_task()) boot_failure("idle/init task creation");
    printf("[task] idle pid=0, init pid=1 created.\n");
    lapic_timer_init_periodic(0x20, 0x10000, 128);
    printf("[task] timer configured; first schedule.\n");
    schedule();
    boot_failure("unexpected return to boot context");
}
