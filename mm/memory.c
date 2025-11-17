#include<p_mem.h>
#include<kmalloc.h>
#include<stdint.h>

void _init_memory()
{
    _init_p_mem();
    _init_kmalloc();
}