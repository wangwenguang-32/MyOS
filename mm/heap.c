#include <heap.h>
#include <physical_memory.h>
#include <kernel.h>
#include <mm_lock.h>
#include <stdint.h>

#define HEAP_MAGIC 0x48454150U
#define LARGE_CLASS 0xffffffffU
#define CLASS_COUNT 8U
/* The object header preserves 8-byte alignment on the 32-bit target. */
typedef struct heap_object {
    uint32_t magic, class_id, pages;
    struct heap_object *next;
} heap_object_t;
static const uint32_t class_sizes[CLASS_COUNT] = {16, 32, 64, 128, 256, 512, 1024, 2048};
static heap_object_t *free_objects[CLASS_COUNT];
static int heap_ready;

void _init_kmalloc(void) { heap_ready = 1; }

uint32_t kmalloc(uint32_t size)
{
    uint32_t flags, class_id, physical, pages, stride, offset;
    heap_object_t *object;
    if (!heap_ready || !size || size > 0xffffffffU - sizeof(*object) - 4095U) return 0;
    flags = mm_irq_save();
    for (class_id = 0; class_id < CLASS_COUNT; class_id++)
        if (size <= class_sizes[class_id]) break;
    if (class_id == CLASS_COUNT) {
        pages = (size + sizeof(*object) + 4095U) >> 12;
        physical = alloc_pages(pages);
        if (!physical) { mm_irq_restore(flags); return 0; }
        object = (void *)(physical + KERNEL_VIRT_BASE);
        object->class_id = LARGE_CLASS;
        object->pages = pages;
    } else {
        if (!free_objects[class_id]) {
            physical = alloc_page();
            if (!physical) { mm_irq_restore(flags); return 0; }
            stride = sizeof(*object) + class_sizes[class_id];
            for (offset = 0; offset + stride <= 4096; offset += stride) {
                object = (void *)(physical + KERNEL_VIRT_BASE + offset);
                object->magic = 0;
                object->class_id = class_id;
                object->pages = 1;
                object->next = free_objects[class_id];
                free_objects[class_id] = object;
            }
        }
        object = free_objects[class_id];
        free_objects[class_id] = object->next;
    }
    object->magic = HEAP_MAGIC;
    object->next = 0;
    mm_irq_restore(flags);
    return (uint32_t)(object + 1);
}

uint32_t kzalloc(uint32_t size)
{
    uint8_t *pointer = (void *)kmalloc(size);
    uint32_t i;
    if (pointer) for (i = 0; i < size; i++) pointer[i] = 0;
    return (uint32_t)pointer;
}

void kfree(void *pointer)
{
    heap_object_t *object;
    uint32_t flags;
    if (!pointer) return;
    flags = mm_irq_save();
    object = (heap_object_t *)pointer - 1;
    if (object->magic != HEAP_MAGIC) { mm_irq_restore(flags); return; }
    object->magic = 0;
    if (object->class_id == LARGE_CLASS)
        free_pages((uint32_t)object - KERNEL_VIRT_BASE, object->pages);
    else if (object->class_id < CLASS_COUNT) {
        object->next = free_objects[object->class_id];
        free_objects[object->class_id] = object;
    }
    mm_irq_restore(flags);
}
