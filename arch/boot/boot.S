/*  boot.S - bootstrap the kernel */
/*  Copyright (C) 1999, 2001, 2010  Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define ASM_FILE        1
#include "multiboot2.h"

/*  C symbol format. HAVE_ASM_USCORE is defined by configure. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym)                     _ ## sym
#else
# define EXT_C(sym)                     sym
#endif

.global _start  , page_directory_base
/*  The size of our stack (16KB). */
#define STACK_SIZE                      0x4000
.section .boot.text
 .type	_start, @function
_start:
multiboot_header:
        /*  Align 64 bits boundary. */
        .align  8
        /*  magic */
        .long   MULTIBOOT2_HEADER_MAGIC
        /*  ISA: i386 */
        .long   MULTIBOOT_ARCHITECTURE_I386
        /*  Header length. */
        .long   multiboot_header_end - multiboot_header
        /*  checksum */
        .long   -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header))
entry_address_tag_start:        
        .align  8
        .short MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS
        .short MULTIBOOT_HEADER_TAG_OPTIONAL
        .long entry_address_tag_end - entry_address_tag_start
        /*  entry_addr */
        .long multiboot_entry
entry_address_tag_end:
end_tag_start:
        .align  8
        .short MULTIBOOT_HEADER_TAG_END
        .short 0
        .long end_tag_end - end_tag_start
end_tag_end:
multiboot_header_end:
multiboot_entry:
        /*  Initialize the stack pointer. */
        movl    $(stack + STACK_SIZE), %esp
        pushl %ebx
        pushl %eax

        /*  Reset EFLAGS. */
        pushl   $0
        popf

        call _init_gdt

        subl $0x6,%esp
        movl $_gdt,2(%esp)
        movw _gdt_limit,%cx
        movw %cx,(%esp)
        lgdt (%esp)
        addl $0x6,%esp
        
        movw $0x28,%ax
        ltr %ax


        pushl $0x08
        pushl  $_after
        retf


_after:
        movw $0x10,%cx
        movw %cx,%es
        movw %cx,%ss
        movw %cx,%fs
        movw %cx,%gs
        movw %cx,%ds

        subl $0x6,%esp
        movl $_idt, %esi
        subl $0xC0000000, %esi
        movl %esi,2(%esp)
        movl $_idt_limit, %esi
        subl $0xC0000000, %esi
        movl (%esi),%ecx
        movw %cx,(%esp)
        lidt (%esp)
        addl $0x6,%esp
        sti



        pushl $page_table_base
        pushl $page_directory_base
        call _init_pgtable

        movl $page_directory_base ,%eax
        movl %eax,%cr3

        movl %cr0,%eax
        orl $0x80000000,%eax
        movl %eax,%cr0

        



        pushl $(stack + STACK_SIZE)
        pushl $main
        ret

loop:   hlt
        jmp     loop
        /*  Our stack area. */
.section .boot.bss
        .align 16
stack:
        .skip 0x4000, 0
        .align 4096
page_directory_base:
        .skip 4096,0

page_table_base:
        .skip 4096,0
        .skip 4096,0

