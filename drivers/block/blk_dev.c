#include <stdint.h>
#include <common.h>
#include <ahci.h>
#include <dma.h>
#include <block_device.h>
#include "block_internal.h"

static blk_info_t blk_info;
static uint8_t blk_initialized;

void blk_init(void) {
    if (blk_initialized) return;
    // 初始化缓存
    if (blk_cache_init()) return;
    
    // 初始化 AHCI
    if (ahci_init() == 0 ) {
        //printf("AHCI blk controller initialized\n");
        blk_info.present = 1;
        uint64_t capacity = ahci_get_disk_size();
        if (!capacity) { blk_info.present = 0; return; }
        blk_info.sectors = capacity > 0xFFFFFFFFULL ? 0xFFFFFFFFU : (uint32_t)capacity;
        blk_initialized = 1;
        return;
    }
    
    //printf("Failed to initialize blk\n");
    blk_info.present = 0;
}

blk_cache_node_t* blk_cache_get(uint32_t lba) {
    blk_cache_node_t *node;
    if (!blk_initialized || lba >= blk_info.sectors) return NULL;
    node = blk_cache_find(lba);
    if (!node) {
        node = blk_cache_get_free();
        dma_addr_t handle;
        if (!node || dma_map_buffer(node->data, BLK_SECTOR_SIZE, 0xffffffffU, &handle)) return NULL;
        int result = ahci_read_sector(lba, handle);
        dma_unmap_buffer(handle, BLK_SECTOR_SIZE);
        if (result) return NULL;
        blk_cache_insert(node, lba);
    }
    node->ref_count++;
    return node;
}

/* Transport hook used only by cache writeback. DMA storage is contiguous. */
int blk_writeback_sector(uint32_t lba, const uint8_t *data)
{
    if (!blk_initialized || !data || lba >= blk_info.sectors) return -1;
    dma_addr_t handle;
    if (dma_map_buffer(data, BLK_SECTOR_SIZE, 0xffffffffU, &handle)) return -1;
    int result = ahci_write_sector(lba, handle);
    dma_unmap_buffer(handle, BLK_SECTOR_SIZE);
    return result;
}

static int blk_write_sector(uint32_t lba, const uint8_t *buffer)
{
    blk_cache_node_t *node;
    if (!blk_initialized || !buffer || lba >= blk_info.sectors) return -1;
    node = blk_cache_find(lba);
    if (!node) {
        node = blk_cache_get_free();
        if (!node) return -1;
        /* Full-sector writes need no disk read. */
        blk_cache_insert(node, lba);
    }
    memcpy(node->data, buffer, BLK_SECTOR_SIZE);
    node->dirty = 1;
    return 0;
}

int blk_sync(void)
{
    if (!blk_initialized) return -1;
    return blk_cache_writeback();
}

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
    
    return ahci_is_disk_ready();
}

int blk_zero_sectors(uint32_t start_lba, uint32_t sectors) {
    if (!blk_initialized || start_lba >= blk_info.sectors ||
        sectors > blk_info.sectors - start_lba) return -1;
    uint8_t zero_sector[blk_SECTOR_SIZE];
    uint32_t i;
    
    for (i = 0; i < blk_SECTOR_SIZE; i++) {
        zero_sector[i] = 0;
    }
    
    for (i = 0; i < sectors; i++) {
        if (blk_write_sector(start_lba + i, zero_sector) != 0) {
            //printf("Format failed at sector %d\n", start_lba + i);
            return -1;
        }
    }
    
    //printf("Formatted %d sectors starting from LBA %d\n", sectors, start_lba);
    return 0;
}

static int blk_range(uint32_t lba, uint32_t sectors) {
    return blk_initialized && sectors && sectors <= 0xffffffffU / BLK_SECTOR_SIZE && lba < blk_info.sectors &&
           sectors <= blk_info.sectors - lba;
}
int blk_read(uint32_t lba, uint8_t* buffer, uint32_t sectors) {
    uint32_t i;
    if (!buffer || !blk_range(lba, sectors)) return -1;
    for (i = 0; i < sectors; i++) {
        blk_cache_node_t *node = blk_cache_get(lba + i);
        if (!node) return -1;
        memcpy(buffer + i * 512, node->data, 512);
        blk_cache_put(node);
    }
    return 0;
}
int blk_write(uint32_t lba, const uint8_t* buffer, uint32_t sectors) {
    uint32_t i;
    if (!buffer || !blk_range(lba, sectors)) return -1;
    for (i = 0; i < sectors; i++)
        if (blk_write_sector(lba + i, buffer + i * 512)) return -1;
    return 0;
}
blk_info_t *blk_get_info(void) { return &blk_info; }
uint8_t blk_get_mode(void) { return blk_MODE_AHCI; }

const char *blk_get_mode_name(void) { return "AHCI"; }
