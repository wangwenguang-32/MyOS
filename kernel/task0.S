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
		call task0_main
loop1:
	 	movl $3,%eax
		
	 	movl $0x10000000,%ecx
zero1:	
	 	subl $1,%ecx
		jz  zero1
     	jmp  loop1
zi:
		call task1_main
loop2:
	 	movl $4,%eax
	 	
	 	movl $0x10000000,%ecx
zero2:	 
		subl $1,%ecx
	    jz  zero2
     	jmp loop2