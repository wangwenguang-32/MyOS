#include<stdint.h>
#include<printf.h>
#include "block_internal.h"
#include <physical_memory.h>
#include<common.h>
#include<block_device.h>



#define PAGE_SIZE             4096

// ========================
// 块设备驱动配置
// ========================

#define BLK_CACHE_SIZE       128        // 缓存条目数
#define BLK_CACHE_ENABLED    1         // 是否启用缓存

#define PAGE_OFFSET           0xC0000000U
#define va(x)                 ((uint32_t)(x)+PAGE_OFFSET)
#define pa(x)                 ((uint32_t)(x)-PAGE_OFFSET)

// ========================
// 缓存结构定义
// ========================



// 哈希表大小（质数）
#define BLK_CACHE_HASH_SIZE  63

// ========================
// 全局变量
// ========================


// 缓存节点池
blk_cache_node_t* blk_cache_pool;

// 哈希表
static blk_cache_node_t* blk_cache_hash[BLK_CACHE_HASH_SIZE];

// LRU 链表头尾
static blk_cache_node_t* lru_head = NULL;  // 最近使用
static blk_cache_node_t* lru_tail = NULL;  // 最久未使用

// 统计信息
static uint32_t cache_hits = 0;
static uint32_t cache_misses = 0;
static uint32_t cache_evictions = 0;
static uint32_t current_time = 0;

// ========================
// 哈希函数
// ========================
static uint32_t blk_cache_hash_func(uint32_t lba) {
    return lba % BLK_CACHE_HASH_SIZE;
}

// ========================
// LRU 链表操作
// ========================

/**
 * 将节点移到 LRU 链表头部（最近使用）
 */
static void blk_cache_lru_move_to_head(blk_cache_node_t* node) {
    if (node == lru_head) return;
    
    // 从链表中移除
    if (node->lru_prev) {
        node->lru_prev->lru_next = node->lru_next;
    } else {
        /* An unlinked node has no predecessor. */
    }
    
    if (node->lru_next) {
        node->lru_next->lru_prev = node->lru_prev;
    } else {
        if (node == lru_tail) lru_tail = node->lru_prev;
    }
    
    // 插入到头部
    node->lru_prev = NULL;
    node->lru_next = lru_head;
    if (lru_head) {
        lru_head->lru_prev = node;
    }
    lru_head = node;
    
    if (!lru_tail) {
        lru_tail = node;
    }
}

/**
 * 获取 LRU 尾部节点（最久未使用且引用计数为 0）
 */
static blk_cache_node_t* blk_cache_lru_get_evict_candidate(void) {
    blk_cache_node_t* node = lru_tail;
    
    // 从尾部开始查找引用计数为 0 的节点
    while (node) {
        if (node->ref_count == 0) {
            return node;
        }
        node = node->lru_prev;
    }
    
    return NULL;  // 所有节点都被引用
}

// ========================
// 哈希表操作
// ========================

/**
 * 从哈希表中查找节点
 */
blk_cache_node_t* blk_cache_hash_find(uint32_t lba) {
    uint32_t hash_idx = blk_cache_hash_func(lba);
    blk_cache_node_t* node = blk_cache_hash[hash_idx];
    
    while (node) {
        if (node->valid && node->lba == lba) {
            return node;
        }
        node = node->hash_next;
    }
    
    return NULL;
}

/**
 * 将节点插入哈希表
 */
static void blk_cache_hash_insert(blk_cache_node_t* node) {
    uint32_t hash_idx = blk_cache_hash_func(node->lba);
    node->hash_next = blk_cache_hash[hash_idx];
    blk_cache_hash[hash_idx] = node;
}

/**
 * 从哈希表中移除节点
 */
static void blk_cache_hash_remove(blk_cache_node_t* node) {
    uint32_t hash_idx = blk_cache_hash_func(node->lba);
    blk_cache_node_t* curr = blk_cache_hash[hash_idx];
    blk_cache_node_t* prev = NULL;
    
    while (curr) {
        if (curr == node) {
            if (prev) {
                prev->hash_next = curr->hash_next;
            } else {
                blk_cache_hash[hash_idx] = curr->hash_next;
            }
            return;
        }
        prev = curr;
        curr = curr->hash_next;
    }
}

// ========================
// 缓存管理函数
// ========================

/**
 * 初始化缓存系统
 */
int blk_cache_init(void) {
    uint32_t i;
    if (blk_cache_pool) return 0;

    uint32_t pool_pages = (sizeof(blk_cache_node_t) * BLK_CACHE_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t pool_pa = alloc_pages(pool_pages);
    uint32_t data_pa;
    if (!pool_pa) return -1;
    data_pa = alloc_pages(BLK_SECTOR_SIZE * BLK_CACHE_SIZE / PAGE_SIZE);
    if (!data_pa) { free_pages(pool_pa, pool_pages); return -1; }
    blk_cache_pool = (blk_cache_node_t *)va(pool_pa);
    uint8_t *blk_cache_data = (uint8_t *)va(data_pa);
    // 初始化节点池
    for (i = 0; i < BLK_CACHE_SIZE; i++) {
        blk_cache_pool[i].data=blk_cache_data+i*blk_SECTOR_SIZE;
        blk_cache_pool[i].valid = 0;
        blk_cache_pool[i].dirty = 0;
        blk_cache_pool[i].ref_count = 0;
        blk_cache_pool[i].access_time = 0;
        blk_cache_pool[i].lru_prev = NULL;
        blk_cache_pool[i].lru_next = NULL;
        blk_cache_pool[i].hash_next = NULL;
    }
    
    // 初始化哈希表
    for (i = 0; i < BLK_CACHE_HASH_SIZE; i++) {
        blk_cache_hash[i] = NULL;
    }
    
    lru_head = NULL;
    lru_tail = NULL;
    cache_hits = 0;
    cache_misses = 0;
    cache_evictions = 0;
    current_time = 0;
    return 0;
}

/**
 * 查找缓存块（增加引用计数）
 */
blk_cache_node_t* blk_cache_find(uint32_t lba) {
    blk_cache_node_t* node = blk_cache_hash_find(lba);
    
    if (node) {
        cache_hits++;
        node->access_time = current_time++;
        blk_cache_lru_move_to_head(node);
        return node;
    }
    
    cache_misses++;
    return NULL;
}

/**
 * 释放缓存块（减少引用计数）
 */
void blk_cache_put(blk_cache_node_t* node) {
    if (node && node->ref_count > 0) {
        node->ref_count--;
    }
}

/**
 * 获取空闲缓存节点
 */
static int blk_cache_writeback_node(blk_cache_node_t *node)
{
    int result;
    if (!node->valid || !node->dirty) return 0;
    result = blk_writeback_sector(node->lba, node->data);
    if (result) return result; /* Keep dirty data available for retry. */
    node->dirty = 0;
    return 0;
}

int blk_cache_writeback(void)
{
    uint32_t i;
    int result = 0;
    if (!blk_cache_pool) return -1;
    for (i = 0; i < BLK_CACHE_SIZE; i++) {
        int error = blk_cache_writeback_node(&blk_cache_pool[i]);
        if (error && !result) result = error;
    }
    return result;
}

blk_cache_node_t* blk_cache_get_free(void) {
    uint32_t i;
    
    // 先找未使用的节点
    for (i = 0; i < BLK_CACHE_SIZE; i++) {
        if (!blk_cache_pool[i].valid) {
            return &blk_cache_pool[i];
        }
    }
    
    // 尝试驱逐 LRU 节点（引用计数为 0）
    blk_cache_node_t* evict_node = blk_cache_lru_get_evict_candidate();
    if (evict_node) {
        if (blk_cache_writeback_node(evict_node)) return NULL;
        blk_cache_hash_remove(evict_node);
        evict_node->valid = 0;
        evict_node->dirty = 0;
        cache_evictions++;
        return evict_node;
    }
    
    // 无法驱逐（所有节点都被引用）
    return NULL;
}

/**
 * 将数据放入缓存
 */
blk_cache_node_t* blk_cache_insert(blk_cache_node_t* node,uint32_t lba) {
    if (!node) {
        //printf("Cache full: cannot allocate node for LBA 0x%x\n", lba);
        return NULL;
    }
    
    node->lba = lba;
    node->valid = 1;
    node->dirty = 0;
    node->access_time = current_time++;
    
    
    // 插入哈希表和 LRU 链表
    blk_cache_hash_insert(node);
    blk_cache_lru_move_to_head(node);
    
    return node;
}

/**
 * 清空缓存
 */
static void blk_cache_flush(void) {
    uint32_t i;
    
    for (i = 0; i < BLK_CACHE_SIZE; i++) {
        blk_cache_pool[i].valid = 0;
        blk_cache_pool[i].dirty = 0;
        blk_cache_pool[i].ref_count = 0;
        blk_cache_pool[i].lru_prev = NULL;
        blk_cache_pool[i].lru_next = NULL;
        blk_cache_pool[i].hash_next = NULL;
    }
    
    for (i = 0; i < BLK_CACHE_HASH_SIZE; i++) {
        blk_cache_hash[i] = NULL;
    }
    
    lru_head = NULL;
    lru_tail = NULL;
    cache_hits = 0;
    cache_misses = 0;
    cache_evictions = 0;
}



void blk_get_cache_stats(uint32_t* hits, uint32_t* misses) {
    if (hits) *hits = cache_hits;
    if (misses) *misses = cache_misses;
}

/**
 * 获取详细的缓存统计信息
 */
void blk_get_cache_stats_detailed(uint32_t* hits, uint32_t* misses, 
                                    uint32_t* evictions, uint32_t* used_entries) {
    if (hits) *hits = cache_hits;
    if (misses) *misses = cache_misses;
    if (evictions) *evictions = cache_evictions;
    
    if (used_entries) {
        uint32_t count = 0;
        uint32_t i;
        for (i = 0; i < BLK_CACHE_SIZE; i++) {
            if (blk_cache_pool && blk_cache_pool[i].valid) {
                count++;
            }
        }
        *used_entries = count;
    }
}

/**
 * 获取缓存命中率
 */
uint32_t blk_get_cache_hit_rate(void) {
    uint32_t total = cache_hits + cache_misses;
    if (total == 0) return 0;
    return (cache_hits * 100) / total;
}

/**
 * 清空缓存
 */
int blk_cache_clear(void) {
    uint32_t i;
    if (!blk_cache_pool) return 0;
    for (i = 0; i < BLK_CACHE_SIZE; i++)
        if (blk_cache_pool[i].ref_count) return -1;
    if (blk_sync()) return -1;
    blk_cache_flush();
    return 0;
}

/**
 * 打印缓存统计信息
 */
void blk_cache_print_stats(void) {
    uint32_t used = 0;
    uint32_t i;
    
    for (i = 0; i < BLK_CACHE_SIZE; i++) {
        if (blk_cache_pool && blk_cache_pool[i].valid) {
            used++;
        }
    }
    
    //printf("=== Disk Cache Statistics ===\n");
    //printf("Total entries: %d\n", BLK_CACHE_SIZE);
    //printf("Used entries: %d\n", used);
    //printf("Cache hits: %d\n", cache_hits);
    //printf("Cache misses: %d\n", cache_misses);
    //printf("Cache evictions: %d\n", cache_evictions);
    
    uint32_t total = cache_hits + cache_misses;
    if (total > 0) {
        //printf("Hit rate: %d%%\n", (cache_hits * 100) / total);
    }
    //printf("============================\n");
}

/**
 * 打印 LRU 链表信息
 */
void blk_cache_print_lru(void) {
    blk_cache_node_t* node = lru_head;
    uint32_t idx = 0;
    
    //printf("=== LRU Chain (Most Recent -> Least Recent) ===\n");
    while (node && idx < BLK_CACHE_SIZE) {
        if (node->valid) {
            //printf("[%d] LBA=0x%x, RefCnt=%d, Time=%d, Dirty=%d\n",
                //    idx, node->lba, node->ref_count, node->access_time, node->dirty);
        }
        node = node->lru_next;
        idx++;
    }
    //printf("==============================================\n");
}

/**
 * 打印哈希表信息
 */
void blk_cache_print_hash_table(void) {
    uint32_t i;
    uint32_t total_entries = 0;
    
    //printf("=== Hash Table Statistics ===\n");
    //printf("Hash table size: %d\n", BLK_CACHE_HASH_SIZE);
    
    for (i = 0; i < BLK_CACHE_HASH_SIZE; i++) {
        uint32_t chain_len = 0;
        blk_cache_node_t* node = blk_cache_hash[i];
        
        while (node) {
            chain_len++;
            total_entries++;
            node = node->hash_next;
        }
        
        if (chain_len > 0) {
            //printf("Bucket[%d]: %d entries\n", i, chain_len);
        }
    }
    
    //printf("Total entries in hash table: %d\n", total_entries);
    //printf("==============================\n");
}

/**
 * 检查磁盘是否初始化
 */
