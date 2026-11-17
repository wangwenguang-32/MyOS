#ifndef MYOS_HEAP_H
#define MYOS_HEAP_H

#include <stdint.h>

uint32_t kmalloc(uint32_t size);
uint32_t kzalloc(uint32_t size);

void kfree(void* ptr);

void _init_kmalloc(void);

#endif

