#ifndef VIRTUAL_AREA_H
#define VIRTUAL_AREA_H
#include <stdint.h>
#define VAREA_RAM 1U
#define VAREA_IO 2U
struct virtual_area {
    uint32_t start, size, offset, kind;
    struct virtual_area *next;
};
/* Caller holds mm_irq_save protection while accessing records. */
struct virtual_area *virtual_area_reserve(uint32_t size, uint32_t kind);
struct virtual_area *virtual_area_find(uint32_t address, uint32_t kind);
void virtual_area_release(struct virtual_area *area);
int virtual_area_init(void);
#endif
