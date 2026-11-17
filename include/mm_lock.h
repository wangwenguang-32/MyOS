#ifndef MM_LOCK_H
#define MM_LOCK_H
#include <stdint.h>
/* Uniprocessor exclusion only: allocation must not sleep while held. */
static inline uint32_t mm_irq_save(void) {
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli" : "=r"(flags) :: "memory");
    return flags;
}
static inline void mm_irq_restore(uint32_t flags) {
    __asm__ volatile("pushl %0; popfl" :: "r"(flags) : "memory", "cc");
}
#endif
