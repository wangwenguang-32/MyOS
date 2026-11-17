#ifndef  MEMORY_H
#define MEMORY_H
#include <stdint.h>

#define MEM_ZERO(addr, size) \
    do {                            \
        if ( size > 0) { \
            char *__p = (char *)(addr);    \
            uint32_t __i = 0;    \
            for (__i; __i < (size); __i++) { \
                __p[__i] = 0;              \
            }                           \
        }                              \
    } while (0)


typedef enum {
    MEMORY_OFFLINE, MEMORY_INITIALIZING, MEMORY_READY, MEMORY_FAILED
} memory_state_t;
#define MEMORY_ERR_PHYSICAL -1
#define MEMORY_ERR_PAGING -2
#define MEMORY_ERR_VIRTUAL -3
int init_memory(void);
memory_state_t memory_get_state(void);
int memory_get_init_error(void);

#endif