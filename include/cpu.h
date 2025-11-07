#ifndef CPU_H
#define CPU_H

#include<stdint.h>

/* -------------------- CPUID -------------------- */
void cpuid_eax1(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    uint32_t a=*eax, b, c, d;
    __asm__ __volatile__("cpuid"
                         : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                         : "a"(a));
    *eax = a; *ebx = b; *ecx = c; *edx = d;
}

#endif
