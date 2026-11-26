#ifndef MYOS_DIRECT_MAP_H
#define MYOS_DIRECT_MAP_H
#include <stdint.h>
#include <paging.h>
/* Bootstrap mapping covers the first 8 MiB before the final CR3 switch. */
#define BOOT_DIRECT_MAP_SIZE (8U * 1024 * 1024)
int direct_map_build(page_directory_t *directory);
void direct_map_activate(void);
int direct_map_is_ready(void);
/* Range checks describe mappings, not RAM availability or page ownership. */
int direct_map_contains_physical(uint32_t address, uint32_t size);
void *direct_map_pointer(uint32_t physical);
int direct_map_physical(const void *pointer, uint32_t *physical);
#endif
