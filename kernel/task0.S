#define ASSEMBLY
#include <task0.h>

TASK0_TEXT
task0_start:
        movl $0x23,%eax
		movw %ax,%ds
		movw %ax,%es
		movw %ax,%fs
		movw %ax,%gs
		call print_hello1
        jmp task0_start