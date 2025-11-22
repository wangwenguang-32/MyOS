#ifndef  MEMORY_H
#define MEMORY_H

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


#endif