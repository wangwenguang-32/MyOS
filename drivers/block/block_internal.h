#ifndef MYOS_BLOCK_INTERNAL_H
#define MYOS_BLOCK_INTERNAL_H
#include <block_device.h>
int blk_cache_init(void);
blk_cache_node_t *blk_cache_hash_find(uint32_t lba);
blk_cache_node_t *blk_cache_find(uint32_t lba);
blk_cache_node_t *blk_cache_get_free(void);
blk_cache_node_t *blk_cache_insert(blk_cache_node_t *node, uint32_t lba);
int blk_writeback_sector(uint32_t lba, const uint8_t *data);
int blk_cache_writeback(void);
#endif
