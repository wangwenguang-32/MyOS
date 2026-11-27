#include <process.h>
#include <scheduler.h>
#include <vga.h>
typedef int (*syscall_handler_t)(void);
extern int sys_fork(void);
extern int sys_putchar_entry(void);
int sys_putchar_value(unsigned int value)
{
    vga_putchar((uint8_t)value);
    return 0;
}
static int sys_yield(void) { schedule(); return 0; }
static int sys_wait(void)
{
    current->state = TASK_INTERRUPTIBLE;
    schedule();
    return 0;
}
/* 0 unsupported, 1 yield, 2 fork disabled until VMA/COW support is complete,
 * 3 putchar (byte in EBX), 4 wait until explicitly woken. */
syscall_handler_t system_func_table[5] = {0, sys_yield, 0, sys_putchar_entry, sys_wait};
