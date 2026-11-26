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

.global _start, multiboot_entry, page_directory_base
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
        cli
        cld
        movl $(stack + STACK_SIZE), %esp
        xorl %ebp, %ebp
        # Preserve Multiboot registers across C bootstrap helpers.
        movl %eax, %edi
        movl %ebx, %ebp
        cmpl $MULTIBOOT2_BOOTLOADER_MAGIC, %edi
        jne boot_halt
        call _init_gdt
        lgdt boot_gdtr
        ljmp $0x08, $boot_segments
boot_segments:
        movw $0x10, %ax
        movw %ax, %ds
        movw %ax, %es
        movw %ax, %ss
        movw %ax, %fs
        movw %ax, %gs
        # i386 cdecl: align stack before calling the bootstrap helper.
        subl $8, %esp
        pushl $page_table_base
        pushl $page_directory_base
        call _init_pgtable
        addl $16, %esp
        movl $page_directory_base, %eax
        movl %eax, %cr3
        movl %cr0, %eax
        orl $0x80000000, %eax
        movl %eax, %cr0
        movl $boot_high_entry, %eax
        jmp *%eax
boot_halt:
        cli
        hlt
        jmp boot_halt

.section .text
boot_high_entry:
        # All kernel C code starts with a high virtual stack, never changes ESP in C.
        movl $(stack + STACK_SIZE + 0xC0000000), %esp
        lgdt kernel_gdtr
        # Explicitly clear kernel BSS before accessing global initialization state.
        movl %edi, %esi
        movl $_bss_start, %edi
        movl $_bss_end, %ecx
        subl %edi, %ecx
        xorl %eax, %eax
        rep stosb
        movl %esi, %edi
        lidt kernel_idtr
        movw $0x28, %ax
        ltr %ax
        # void main(uint32_t magic, uint32_t boot_info_physical)
        subl $8, %esp
        pushl %ebp
        pushl %edi
        xorl %ebp, %ebp
        call main
        cli
1:      hlt
        jmp 1b

.section .boot.data
boot_gdtr:
        .word 6 * 8 - 1
        .long _gdt
kernel_gdtr:
        .word 6 * 8 - 1
        .long _gdt + 0xC0000000
kernel_idtr:
        .word 256 * 8 - 1
        .long _idt + 0xC0000000
        /*  Our stack area. */
.section .boot.data
        .align 16
stack:
        .skip 0x4000, 0
        .align 4096
page_directory_base:
        .skip 4096,0

page_table_base:
        .skip 4096,0
        .skip 4096,0
        .skip 4096,0

