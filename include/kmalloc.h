#ifndef KMALLOC_H
#define KMALLOC_H

#include <stdint.h>

void* kmalloc(uint32_t size);

void kfree(void* ptr);

void init_kmalloc(void);

#endif

