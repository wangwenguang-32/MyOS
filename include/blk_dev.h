#ifndef BLK_DEV_H
#define BLK_DEV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================
// 数据结构
// ========================
// 缓存条目结构
typedef struct __attribute__((packed)) blk_cache_node {
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
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors_per_track;
} blk_info_t;

#define blk_SECTOR_SIZE 512

// ========================
// 驱动模式
// ========================
#define blk_MODE_AHCI   2

// ========================
// 基础接口
// ========================

/**
 * 初始化磁盘驱动（自动检测 AHCI/IDE）
 */
void blk_init(void);

/**
 * 读取单个扇区
 * @param lba 逻辑块地址
 * @param buffer 缓冲区指针（至少 512 字节）
 * @return 0 成功，负数表示错误
 */
blk_cache_node_t* blk_read(uint32_t lba);

/**
 * 写入单个扇区
 * @param lba 逻辑块地址
 * @param buffer 缓冲区指针（包含要写入的数据）
 * @return 0 成功，负数表示错误
 */
int blk_write(uint32_t lba, uint8_t* buffer);

/**
 * 读取多个扇区
 * @param lba 起始逻辑块地址
 * @param buffer 缓冲区指针
 * @param sectors 扇区数量
 * @return 0 成功，负数表示错误
 */
int blk_read_multiple(uint32_t lba, uint8_t* buffer, uint32_t sectors);

/**
 * 写入多个扇区
 * @param lba 起始逻辑块地址
 * @param buffer 缓冲区指针
 * @param sectors 扇区数量
 * @return 0 成功，负数表示错误
 */
int blk_write_multiple(uint32_t lba, uint8_t* buffer, uint32_t sectors);

/**
 * 获取磁盘信息
 * @return 指向 blk_info_t 结构的指针
 */
blk_info_t* blk_get_info(void);

/**
 * 格式化磁盘区域（写入零）
 * @param start_lba 起始逻辑块地址
 * @param sectors 扇区数量
 * @return 0 成功，负数表示错误
 */
int blk_format(uint32_t start_lba, uint32_t sectors);

// ========================
// 扩展接口
// ========================

/**
 * 获取当前驱动模式
 * @return blk_MODE_AHCI 或 blk_MODE_IDE
 */
uint8_t blk_get_mode(void);

/**
 * 获取驱动模式名称
 * @return 模式名称字符串
 */
const char* blk_get_mode_name(void);

/**
 * 获取缓存统计信息
 * @param hits 缓存命中次数指针
 * @param misses 缓存未命中次数指针
 */
void blk_get_cache_stats(uint32_t* hits, uint32_t* misses);

/**
 * 获取详细的缓存统计信息
 * @param hits 缓存命中次数
 * @param misses 缓存未命中次数
 * @param evictions 缓存驱逐次数
 * @param used_entries 已使用的缓存条目数
 */
void blk_get_cache_stats_detailed(uint32_t* hits, uint32_t* misses, 
                                    uint32_t* evictions, uint32_t* used_entries);

/**
 * 获取缓存命中率（百分比）
 */
uint32_t blk_get_cache_hit_rate(void);

/**
 * 清空缓存
 */
void blk_cache_clear(void);

/**
 * 打印缓存统计信息
 */
void blk_cache_print_stats(void);

/**
 * 打印 LRU 链表信息
 */
void blk_cache_print_lru(void);

/**
 * 打印哈希表信息
 */
void blk_cache_print_hash_table(void);

/**
 * 检查磁盘是否初始化
 * @return 1 已初始化，0 未初始化
 */
int blk_is_initialized(void);

/**
 * 检查磁盘是否就绪
 * @return 1 就绪，0 未就绪
 */
int blk_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif
