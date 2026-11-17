#include <memory.h>
#include <physical_memory.h>
#include <heap.h>
#include <io_mapping.h>
#include <paging.h>
#include <mm_lock.h>

static memory_state_t state;
static int initialization_error;

memory_state_t memory_get_state(void) { return state; }
int memory_get_init_error(void) { return initialization_error; }

int init_memory(void)
{
    uint32_t flags = mm_irq_save();
    int result = 0;
    if (state == MEMORY_READY) { mm_irq_restore(flags); return 0; }
    if (state != MEMORY_OFFLINE) {
        result = initialization_error ? initialization_error : -1;
        mm_irq_restore(flags);
        return result;
    }
    state = MEMORY_INITIALIZING;
    if (_init_phy_mem()) result = MEMORY_ERR_PHYSICAL;
    else if (_init_paging()) result = MEMORY_ERR_PAGING;
    else if (_init_iomap()) result = MEMORY_ERR_VIRTUAL;
    else {
        _init_kmalloc();
        state = MEMORY_READY;
    }
    if (result) { initialization_error = result; state = MEMORY_FAILED; }
    mm_irq_restore(flags);
    return result;
}
