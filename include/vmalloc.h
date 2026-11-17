#ifndef VMALLOC_H
#define VMALLOC_H
#include <stdint.h>
void *vmalloc(uint32_t size);
void *vzalloc(uint32_t size);
void vfree(void *pointer);
#endif
