/*  kernel.c - the C part of the kernel */
/*  Copyright (C) 1999, 2010  Free Software Foundation, Inc.
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

#include "multiboot2.h"
#include <printf.h>
/*  Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
multiboot_info_process (unsigned long magic, unsigned long addr)
{  
  struct multiboot_tag *tag;
  unsigned size;
  /*  Am I booted by a Multiboot-compliant boot loader? */
  if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
      //printf ("Invalid magic number: 0x%x\n", (unsigned) magic);
      return;
    }
  if (addr & 7)
    {
      //printf ("Unaligned mbi: 0x%x\n", addr);
      return;
    }

  size = *(unsigned *) addr;
  //printf ("Announced mbi size 0x%x\n", size);
  for (tag = (struct multiboot_tag *) (addr + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag 
                                       + ((tag->size + 7) & ~7)))
    {

      //printf ("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);
      switch (tag->type)
        {
        case MULTIBOOT_TAG_TYPE_CMDLINE:
          // printf ("Command line = %s\n",
          //         ((struct multiboot_tag_string *) tag)->string);
          break;
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
          // printf ("Boot loader name = %s\n",
          //         ((struct multiboot_tag_string *) tag)->string);
          break;
        case MULTIBOOT_TAG_TYPE_MODULE:
          // printf ("Module at 0x%x-0x%x. Command line %s\n",
          //         ((struct multiboot_tag_module *) tag)->mod_start,
          //         ((struct multiboot_tag_module *) tag)->mod_end,
          //         ((struct multiboot_tag_module *) tag)->cmdline);
          break;
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
          // printf ("mem_lower = %uKB, mem_upper = %uKB\n",
          //         ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
          //         ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);
          break;
        case MULTIBOOT_TAG_TYPE_BOOTDEV:
          // printf ("Boot device 0x%x,%u,%u\n",
          //         ((struct multiboot_tag_bootdev *) tag)->biosdev,
          //         ((struct multiboot_tag_bootdev *) tag)->slice,
          //         ((struct multiboot_tag_bootdev *) tag)->part);
          break;
        case MULTIBOOT_TAG_TYPE_MMAP:
          {
            setup_mmap(tag);
          }
          break;

        }
    }
  tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag 
                                  + ((tag->size + 7) & ~7));
  //printf ("Total mbi size 0x%x\n", (unsigned) tag - addr);
}    


