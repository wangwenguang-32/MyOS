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
		movl $2,%eax
		int $0x80
		cmpl $0,%eax
		je zi
loop1:	
     jmp  loop1
zi:
loop2:		
     jmp loop2