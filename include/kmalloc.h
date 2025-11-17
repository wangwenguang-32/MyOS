#ifndef KMALLOC_H
#define KMALLOC_H

#include <stdint.h>

uint32_t kmalloc(uint32_t size);

void kfree(void* ptr);

void _init_kmalloc(void);

#endif

