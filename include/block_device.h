#ifndef MYOS_BLOCK_DEVICE_H
#define MYOS_BLOCK_DEVICE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct blk_cache_node {
    uint32_t lba;                           // 逻辑块地址
    uint8_t* data;                          // 数据
    uint16_t valid;                        // 是否有效
    uint16_t dirty;                        // 是否脏                        // 
    uint32_t ref_count;                     // 引用计数
    uint32_t access_time;                   // 访问时间戳
    struct blk_cache_node* lru_prev;       // LRU 链表前驱
    struct blk_cache_node* lru_next;       // LRU 链表后继
    struct blk_cache_node* hash_next;      // 哈希链表后继
} blk_cache_node_t;

typedef struct {
    uint8_t present;
    uint32_t sectors;
} blk_info_t;


#define BLK_SECTOR_SIZE 512U
#define blk_SECTOR_SIZE BLK_SECTOR_SIZE
#define blk_MODE_AHCI 2
void blk_init(void);
/* Buffer I/O: zero on success, negative on failure. Partial I/O is possible. */
int blk_read(uint32_t lba, uint8_t *buffer, uint32_t sectors);
int blk_write(uint32_t lba, const uint8_t *buffer, uint32_t sectors);
/* Write/zero success means cached, not durable. Calls must be serialized.
 * Sync waits for DMA writes; ATA device-cache flush is not yet implemented. */
int blk_sync(void);
int blk_zero_sectors(uint32_t lba, uint32_t sectors);
/* Cache nodes must be returned with blk_cache_put; data is read-only to callers. */
blk_cache_node_t *blk_cache_get(uint32_t lba);
void blk_cache_put(blk_cache_node_t *node);
blk_info_t *blk_get_info(void);
uint8_t blk_get_mode(void);
const char *blk_get_mode_name(void);
int blk_is_initialized(void);
int blk_is_ready(void);
void blk_get_cache_stats(uint32_t *hits, uint32_t *misses);
void blk_get_cache_stats_detailed(uint32_t *hits, uint32_t *misses,
                                uint32_t *evictions, uint32_t *used_entries);
uint32_t blk_get_cache_hit_rate(void);
/* Sync dirty sectors before invalidating; fail if pinned or writeback fails. */
int blk_cache_clear(void);
void blk_cache_print_stats(void);
void blk_cache_print_lru(void);
void blk_cache_print_hash_table(void);
#ifdef __cplusplus
}
#endif
#endif
