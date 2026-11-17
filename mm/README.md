# Memory management

- physical_memory.c: Multiboot memory map and contiguous physical page allocator.
- paging.c: x86 page tables and kernel direct mapping.
- heap.c: 8-byte-aligned kernel heap, address-ordered free blocks and coalescing.
- virtual_memory.c: process virtual memory areas.
- io_mapping.c: uncached MMIO virtual mappings.
- memory_init.c: initialization order.

Public header names remain compatible with existing callers. page.c is an empty
legacy translation unit and is no longer linked.

Physical allocation returns zero on exhaustion. Physical pages currently come
from the direct-map range only. Boot memory-map entries outside 32-bit physical
address space are ignored. Only fully usable pages are made available; reserved
entries override usable entries.

Heap allocations return zero on failure. kfree accepts only a live pointer
returned by kmalloc, exactly once. Freed heap blocks remain available to the heap;
pages are not returned to the physical allocator. Allocators require serialized
calls; interrupt/SMP locking is not implemented.

Required runtime verification: boot with different RAM sizes, physical allocator
exhaustion, contiguous allocations, heap split/coalesce and MMIO mapping failures.
Existing process COW and VMA lifetime paths require a separate runtime audit.
