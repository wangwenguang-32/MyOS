#ifndef DMA_H
#define DMA_H
#include <stdint.h>
typedef uint32_t dma_addr_t;
/* x86 identity-DMA backend, no IOMMU. mask is the device's maximum address. */
void *dma_alloc_coherent(uint32_t size, uint32_t mask, dma_addr_t *handle);
void dma_free_coherent(void *pointer);
/* Map a direct-map, physically contiguous buffer. Returns 0 on success. */
int dma_map_buffer(const void *pointer, uint32_t size, uint32_t mask, dma_addr_t *handle);
void dma_unmap_buffer(dma_addr_t handle, uint32_t size);
#endif
