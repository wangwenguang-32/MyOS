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

#include <multiboot2.h>
#include <boot_info.h>
#include <printf.h>
#include <physical_memory.h>
#include <direct_map.h>

static void print_string(const char *label, const char *text, uint32_t capacity)
{
    char buffer[128];
    uint32_t i = 0;
    while (i < capacity && i + 1 < sizeof(buffer) && text[i]) {
        unsigned char c = text[i];
        buffer[i++] = c >= 32 && c < 127 ? c : '?';
    }
    buffer[i] = 0;
    printf("%s%s\n", label, buffer);
}

/* Print full-width addresses without relying on unsupported printf widths/ll. */
static void print_hex64(uint64_t value)
{
    static const char digits[] = "0123456789abcdef";
    char buffer[19];
    uint32_t i;
    buffer[0] = '0'; buffer[1] = 'x';
    for (i = 0; i < 16; i++) buffer[2 + i] = digits[(value >> ((15 - i) * 4)) & 15];
    buffer[18] = 0;
    printf("%s", buffer);
}

int multiboot_info_process(unsigned long magic, unsigned long physical)
{
    uint32_t size, offset, modules = 0, regions = 0;
    uint8_t *base;
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC || (physical & 7) ||
        !direct_map_contains_physical(physical, 8)) return -1;
    base = direct_map_pointer(physical);
    size = *(uint32_t *)base;
    if (size < 16 || (size & 7U) || !direct_map_contains_physical(physical, size)) return -1;
    reserve_boot_memory(physical, physical + size);
    printf("[boot] Multiboot2 info: physical=0x%x bytes=%u\n", physical, size);
    for (offset = 8; offset <= size - 8;) {
        struct multiboot_tag *tag = (void *)(base + offset);
        if (tag->size < 8 || tag->size > size - offset) return -1;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            if (tag->size != 8) return -1;
            printf("[boot] Modules=%u memory regions=%u\n", modules, regions);
            return regions ? 0 : -1;
        }
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_CMDLINE:
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
            print_string(tag->type == MULTIBOOT_TAG_TYPE_CMDLINE ?
                "[boot] Command line: " : "[boot] Loader: ",
                (const char *)tag + 8, tag->size - 8);
            break;
        case MULTIBOOT_TAG_TYPE_MODULE: {
            struct multiboot_tag_module *module = (void *)tag;
            if (tag->size < sizeof(*module) || module->mod_end <= module->mod_start) return -1;
            reserve_boot_memory(module->mod_start, module->mod_end);
            printf("[boot] Module: 0x%x..0x%x\n", module->mod_start, module->mod_end);
            print_string("       name: ", module->cmdline, tag->size - sizeof(*module));
            modules++;
            break;
        }
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
            struct multiboot_tag_basic_meminfo *info = (void *)tag;
            if (tag->size < sizeof(*info)) return -1;
            printf("[boot] Basic memory: lower=%u KiB upper=%u KiB\n",
                   info->mem_lower, info->mem_upper);
            break;
        }
        case MULTIBOOT_TAG_TYPE_MMAP: {
            struct multiboot_tag_mmap *map = (void *)tag;
            uint32_t position;
            if (tag->size < sizeof(*map) || map->entry_size < sizeof(multiboot_memory_map_t) ||
                (tag->size - sizeof(*map)) % map->entry_size || regions) return -1;
            printf("[boot] Physical memory map (base, length, type):\n");
            for (position = sizeof(*map); position < tag->size; position += map->entry_size) {
                multiboot_memory_map_t *entry = (void *)((uint8_t *)map + position);
                printf("  "); print_hex64(entry->addr);
                printf(" + "); print_hex64(entry->len);
                printf(" %s (%u)\n", addr_range_type_str(entry->type), entry->type);
                regions++;
            }
            setup_mmap(map);
            break;
        }
        default: break;
        }
        uint32_t step = (tag->size + 7U) & ~7U;
        if (step > size - offset) return -1;
        offset += step;
    }
    return -1; /* Missing end tag. */
}
