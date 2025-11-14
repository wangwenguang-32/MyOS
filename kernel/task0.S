#define ASSEMBLY
#include <task0.h>
#include <section.h>

TASK0_TEXT
task0_start:
        movl $0x23,%eax
		movw %ax,%ds
		movw %ax,%es
		movw %ax,%fs
		movw %ax,%gs
 loop:    jmp loop