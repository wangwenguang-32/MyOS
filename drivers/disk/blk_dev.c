#include<stdint.h>
#include<printf.h>
#include<ahci.h>
#include<string.h>
#include<blk_dev.h>

#define BLK_SECTOR_SIZE      512
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
blk_info_t blk_info;
static uint8_t blk_initialized = 0;

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
        lru_tail = node->lru_next;
    }
    
    if (node->lru_next) {
        node->lru_next->lru_prev = node->lru_prev;
    } else {
        lru_head = node->lru_prev;
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
static blk_cache_node_t* blk_cache_hash_find(uint32_t lba) {
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
static void blk_cache_init(void) {
    uint32_t i;

    blk_cache_pool=(blk_cache_node_t*)va(alloc_pages((sizeof(blk_cache_node_t)*BLK_CACHE_SIZE)/PAGE_SIZE));
    uint8_t*blk_cache_data=(uint8_t*)va(alloc_pages((BLK_SECTOR_SIZE*BLK_CACHE_SIZE)/PAGE_SIZE));
    
    // 初始化节点池
    for (i = 0; i < BLK_CACHE_SIZE; i++) {
        blk_cache_pool[i].data=(uint32_t)blk_cache_data+i*blk_SECTOR_SIZE;
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
}

/**
 * 查找缓存块（增加引用计数）
 */
static blk_cache_node_t* blk_cache_find(uint32_t lba) {
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
void blk_cache_release(blk_cache_node_t* node) {
    if (node && node->ref_count > 0) {
        node->ref_count--;
    }
}

/**
 * 获取空闲缓存节点
 */
static blk_cache_node_t* blk_cache_get_free(void) {
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
static blk_cache_node_t* blk_cache_put(blk_cache_node_t* node,uint32_t lba) {
    if (!node) {
        printf("Cache full: cannot allocate node for LBA 0x%x\n", lba);
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



void blk_init(void) {
    // 初始化缓存
    blk_cache_init();
    
    // 初始化 AHCI
    if (ahci_init() == 0 ) {
        printf("AHCI blk controller initialized\n");
        blk_info.present = 1;
        blk_info.sectors = 0x100000000ULL;
        blk_initialized = 1;
        return;
    }
    
    printf("Failed to initialize blk\n");
    blk_info.present = 0;
}

blk_cache_node_t* blk_read(uint32_t lba) {
    // if (!blk_info.present) {
    //     return -1;
    // }

    // 尝试从缓存读取
    if (BLK_CACHE_ENABLED) {
        blk_cache_node_t* node = blk_cache_find(lba);
        if (node) {
            node->ref_count++;
            return node;
        }
    }
    
    // 使用 AHCI 读取
    blk_cache_node_t*node=blk_cache_get_free();
    uint32_t buf_pa = pa(node->data);
    int ret = ahci_read_sector(lba, buf_pa);
    node->ref_count++;
    blk_cache_put(node,lba);
    return node;

}

int blk_write(uint32_t lba, uint8_t* buffer) {
    if (!blk_info.present) {
        return -1;
    }

    // 使用 AHCI 写入
    // uint32_t buf_pa = (uint32_t)buffer - KERNEL_BASE;
    // int ret = ahci_write_sector(lba, buf_pa);

    // 更新缓存
    if (BLK_CACHE_ENABLED) {
        // 先从哈希表中查找并移除旧的缓存
        blk_cache_node_t* old_node = blk_cache_find(lba);
        if (old_node) {
            // blk_cache_hash_remove(old_node);
            // old_node->valid = 0;
            old_node->dirty=1;
            memcpy(old_node->data,buffer,blk_SECTOR_SIZE);
            return 1;

        }
        
    }

    blk_cache_node_t*node=blk_cache_get_free();
    memcpy(node->data,buffer,blk_SECTOR_SIZE);
    node->dirty=1;
    node->lba=lba;
    blk_cache_put(node,lba);


    return 1;
}


// ========================
// 扩展接口函数
// ========================

/**
 * 获取驱动模式名称
 */
const char* blk_get_mode_name(void) {
    return "AHCI";
}

/**
 * 获取缓存统计信息
 */
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
            if (blk_cache_pool[i].valid) {
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
void blk_cache_clear(void) {
    blk_cache_flush();
}

/**
 * 打印缓存统计信息
 */
void blk_cache_print_stats(void) {
    uint32_t used = 0;
    uint32_t i;
    
    for (i = 0; i < BLK_CACHE_SIZE; i++) {
        if (blk_cache_pool[i].valid) {
            used++;
        }
    }
    
    printf("=== Disk Cache Statistics ===\n");
    printf("Total entries: %d\n", BLK_CACHE_SIZE);
    printf("Used entries: %d\n", used);
    printf("Cache hits: %d\n", cache_hits);
    printf("Cache misses: %d\n", cache_misses);
    printf("Cache evictions: %d\n", cache_evictions);
    
    uint32_t total = cache_hits + cache_misses;
    if (total > 0) {
        printf("Hit rate: %d%%\n", (cache_hits * 100) / total);
    }
    printf("============================\n");
}

/**
 * 打印 LRU 链表信息
 */
void blk_cache_print_lru(void) {
    blk_cache_node_t* node = lru_head;
    uint32_t idx = 0;
    
    printf("=== LRU Chain (Most Recent -> Least Recent) ===\n");
    while (node && idx < BLK_CACHE_SIZE) {
        if (node->valid) {
            printf("[%d] LBA=0x%x, RefCnt=%d, Time=%d, Dirty=%d\n",
                   idx, node->lba, node->ref_count, node->access_time, node->dirty);
        }
        node = node->lru_next;
        idx++;
    }
    printf("==============================================\n");
}

/**
 * 打印哈希表信息
 */
void blk_cache_print_hash_table(void) {
    uint32_t i;
    uint32_t total_entries = 0;
    
    printf("=== Hash Table Statistics ===\n");
    printf("Hash table size: %d\n", BLK_CACHE_HASH_SIZE);
    
    for (i = 0; i < BLK_CACHE_HASH_SIZE; i++) {
        uint32_t chain_len = 0;
        blk_cache_node_t* node = blk_cache_hash[i];
        
        while (node) {
            chain_len++;
            total_entries++;
            node = node->hash_next;
        }
        
        if (chain_len > 0) {
            printf("Bucket[%d]: %d entries\n", i, chain_len);
        }
    }
    
    printf("Total entries in hash table: %d\n", total_entries);
    printf("==============================\n");
}

/**
 * 检查磁盘是否初始化
 */
int blk_is_initialized(void) {
    return blk_initialized;
}

/**
 * 检查磁盘是否就绪
 */
int blk_is_ready(void) {
    if (!blk_info.present) {
        return 0;
    }
    
    // return ahci_is_blk_ready();
}

int blk_format(uint32_t start_lba, uint32_t sectors) {
    uint8_t zero_sector[blk_SECTOR_SIZE];
    uint32_t i;
    
    for (i = 0; i < blk_SECTOR_SIZE; i++) {
        zero_sector[i] = 0;
    }
    
    for (i = 0; i < sectors; i++) {
        if (blk_write(start_lba + i, zero_sector) != 0) {
            printf("Format failed at sector %d\n", start_lba + i);
            return -1;
        }
    }
    
    printf("Formatted %d sectors starting from LBA %d\n", sectors, start_lba);
    return 0;
}

int blk_read_multiple(uint32_t lba, uint8_t* buffer, uint32_t sectors) {
    if (!blk_info.present) {
        return -1;
    }

    // 使用 AHCI 一次读取多个扇区
    uint32_t buf_pa = (uint32_t)buffer - PAGE_OFFSET;
    int ret = ahci_read_sectors(lba, sectors, buf_pa);
    
    // 缓存第一个扇区
    if (ret == 0 && BLK_CACHE_ENABLED) {
        blk_cache_node_t* node = blk_cache_put(lba, buffer);
        if (node) {
            blk_cache_release(node);
        }
    }
    return ret;
}

int blk_write_multiple(uint32_t lba, uint8_t* buffer, uint32_t sectors) {
    if (!blk_info.present) {
        return -1;
    }

    // 使用 AHCI 一次写入多个扇区
    uint32_t buf_pa = (uint32_t)buffer - PAGE_OFFSET;
    int ret = ahci_write_sectors(lba, sectors, buf_pa);
    
    // 更新缓存
    if (ret == 0 && BLK_CACHE_ENABLED) {
        blk_cache_node_t* old_node = blk_cache_hash_find(lba);
        if (old_node) {
            blk_cache_hash_remove(old_node);
            old_node->valid = 0;
        }
        
        blk_cache_node_t* node = blk_cache_put(lba, buffer);
        if (node) {
            blk_cache_release(node);
        }
    }
    return ret;
}
