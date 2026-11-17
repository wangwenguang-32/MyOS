#ifndef  MYOS_PAGE_REFCOUNT_H
#define MYOS_PAGE_REFCOUNT_H

#include<stdint.h>
#include<list.h>

typedef struct  {
    uint64_t address_count;
    list_node_t node;
} phys_mem_refcount_t;

void insert_count_node(uint32_t addr);
uint8_t add_count(uint32_t addr);
uint8_t del_count(uint32_t addr);
uint32_t get_count(uint32_t addr);


#endif