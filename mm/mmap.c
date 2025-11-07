#include<mmap.h>
#include<printf.h>
#include<multiboot2.h>
struct memory_map  mmap[MAX_MAP_NUM];
int num_mmap=0;

void setup_mmap(struct multiboot_tag_mmap *tag)
{
            multiboot_memory_map_t *map;
            
            for (map = ((struct multiboot_tag_mmap *) tag)->entries;
                 (multiboot_uint8_t *) map 
                   < (multiboot_uint8_t *) tag + tag->size;
                 map = (multiboot_memory_map_t *) 
                   ((unsigned long) map
                    + ((struct multiboot_tag_mmap *) tag)->entry_size))
                    {
                      printf (" [base_addr = 0x%x%x, length = 0x%x%x] %s\n",
                      (unsigned) (map->addr >> 32),
                      (unsigned) (map->addr & 0xffffffff),
                      (unsigned) (map->len >> 32),
                      (unsigned) (map->len & 0xffffffff),addr_range_type_str(map->type));
                      mmap[num_mmap].base_addr= (map->addr & 0xffffffff);
                      mmap[num_mmap].length= (map->len & 0xffffffff);
                      mmap[num_mmap].type=map->type;
                      num_mmap++;
                    }

            int i=0;
            
              

}