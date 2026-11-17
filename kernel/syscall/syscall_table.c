/* Keep syscall numbers in sync with syscall_entry.S (five entries). */
typedef int (*syscall_handler_t)(void);
extern int sys_fork(void);

int sys_write1(void) { return -38; }
int sys_write2(void) { return -38; }

syscall_handler_t system_func_table[5] = {
    0, 0, sys_fork, sys_write1, sys_write2
};
