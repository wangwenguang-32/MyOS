#include "ext2.h"
#include "blk_dev.h"
#include "kmalloc.h"
#include "printf.h"
#include "string.h"
#include <stddef.h>

// ========================
// 全局变量
// ========================

static ext2_fs_t g_ext2_fs = {0};
static uint32_t g_ext2_start_lba = 0;
static int g_ext2_initialized = 0;

// ========================
// 辅助函数
// ========================

/**
 * 计算块组号
 */
static uint32_t ext2_get_group_num(uint32_t inode_num) {
    return (inode_num - 1) / g_ext2_fs.inodes_per_group;
}

/**
 * 计算inode在块组内的索引
 */
static uint32_t ext2_get_inode_index(uint32_t inode_num) {
    return (inode_num - 1) % g_ext2_fs.inodes_per_group;
}

/**
 * 计算块号对应的块组
 */
static uint32_t ext2_get_block_group(uint32_t block_num) {
    return block_num / g_ext2_fs.blocks_per_group;
}

/**
 * 从磁盘读取块
 */
static int ext2_read_block(uint32_t block_num, uint8_t *buffer) {
    uint32_t lba = g_ext2_start_lba + (block_num * g_ext2_fs.block_size) / 512;
    uint32_t sectors = g_ext2_fs.block_size / 512;
    return blk_read(lba, buffer, sectors);
}

/**
 * 向磁盘写入块
 */
static int ext2_write_block(uint32_t block_num, uint8_t *buffer) {
    uint32_t lba = g_ext2_start_lba + (block_num * g_ext2_fs.block_size) / 512;
    uint32_t sectors = g_ext2_fs.block_size / 512;
    return blk_write(lba, buffer, sectors);
}

/**
 * 分配新块
 */
static int ext2_allocate_block(void) {
    uint8_t *bitmap = (uint8_t *)kmalloc(g_ext2_fs.block_size);
    if (!bitmap) return -1;

    // 遍历块组查找空闲块
    for (uint32_t g = 0; g < g_ext2_fs.groups_count; g++) {
        if (g_ext2_fs.group_descs[g].free_blocks_count == 0) continue;

        // 读取块位图
        if (ext2_read_block(g_ext2_fs.group_descs[g].block_bitmap, bitmap) < 0) {
            kfree(bitmap);
            return -1;
        }

        // 查找第一个空闲块
        for (uint32_t i = 0; i < g_ext2_fs.blocks_per_group; i++) {
            uint32_t byte_idx = i / 8;
            uint32_t bit_idx = i % 8;

            if (!(bitmap[byte_idx] & (1 << bit_idx))) {
                // 标记为已用
                bitmap[byte_idx] |= (1 << bit_idx);
                ext2_write_block(g_ext2_fs.group_descs[g].block_bitmap, bitmap);

                // 更新计数
                g_ext2_fs.group_descs[g].free_blocks_count--;
                g_ext2_fs.superblock->free_blocks_count--;

                kfree(bitmap);
                return g * g_ext2_fs.blocks_per_group + i;
            }
        }
    }

    kfree(bitmap);
    return -1;
}

/**
 * 分配新inode
 */
static int ext2_allocate_inode(void) {
    uint8_t *bitmap = (uint8_t *)kmalloc(g_ext2_fs.block_size);
    if (!bitmap) return -1;

    // 遍历块组查找空闲inode
    for (uint32_t g = 0; g < g_ext2_fs.groups_count; g++) {
        if (g_ext2_fs.group_descs[g].free_inodes_count == 0) continue;

        // 读取inode位图
        if (ext2_read_block(g_ext2_fs.group_descs[g].inode_bitmap, bitmap) < 0) {
            kfree(bitmap);
            return -1;
        }

        // 查找第一个空闲inode
        for (uint32_t i = 0; i < g_ext2_fs.inodes_per_group; i++) {
            uint32_t byte_idx = i / 8;
            uint32_t bit_idx = i % 8;

            if (!(bitmap[byte_idx] & (1 << bit_idx))) {
                // 标记为已用
                bitmap[byte_idx] |= (1 << bit_idx);
                ext2_write_block(g_ext2_fs.group_descs[g].inode_bitmap, bitmap);

                // 更新计数
                g_ext2_fs.group_descs[g].free_inodes_count--;
                g_ext2_fs.superblock->free_inodes_count--;

                kfree(bitmap);
                return g * g_ext2_fs.inodes_per_group + i + 1;
            }
        }
    }

    kfree(bitmap);
    return -1;
}

// ========================
// 核心API实现
// ========================

int ext2_init(uint32_t start_lba) {
    if (g_ext2_initialized) return 0;

    g_ext2_start_lba = start_lba;

    // 分配超级块
    g_ext2_fs.superblock = (ext2_superblock_t *)kmalloc(sizeof(ext2_superblock_t));
    if (!g_ext2_fs.superblock) return -1;

    // 读取超级块（位于块1，偏移1024字节）
    uint8_t *block_buffer = (uint8_t *)kmalloc(4096);
    if (!block_buffer) {
        kfree(g_ext2_fs.superblock);
        return -1;
    }

    if (ext2_read_block(1, block_buffer) < 0) {
        kfree(block_buffer);
        kfree(g_ext2_fs.superblock);
        return -1;
    }

    memcpy(g_ext2_fs.superblock, block_buffer, sizeof(ext2_superblock_t));
    kfree(block_buffer);

    // 验证魔数
    if (g_ext2_fs.superblock->magic != 0xEF53) {
        kfree(g_ext2_fs.superblock);
        return -1;
    }

    // 计算文件系统参数
    g_ext2_fs.block_size = 1024 << g_ext2_fs.superblock->block_size;
    g_ext2_fs.inode_size = g_ext2_fs.superblock->inode_size;
    g_ext2_fs.blocks_per_group = g_ext2_fs.superblock->blocks_per_group;
    g_ext2_fs.inodes_per_group = g_ext2_fs.superblock->inodes_per_group;
    g_ext2_fs.groups_count = (g_ext2_fs.superblock->blocks_count + g_ext2_fs.blocks_per_group - 1) / g_ext2_fs.blocks_per_group;

    // 读取块组描述符表
    uint32_t desc_table_blocks = (g_ext2_fs.groups_count * sizeof(ext2_group_desc_t) + g_ext2_fs.block_size - 1) / g_ext2_fs.block_size;
    uint32_t desc_table_size = g_ext2_fs.groups_count * sizeof(ext2_group_desc_t);

    g_ext2_fs.group_descs = (ext2_group_desc_t *)kmalloc(desc_table_size);
    if (!g_ext2_fs.group_descs) {
        kfree(g_ext2_fs.superblock);
        return -1;
    }

    block_buffer = (uint8_t *)kmalloc(g_ext2_fs.block_size * desc_table_blocks);
    if (!block_buffer) {
        kfree(g_ext2_fs.group_descs);
        kfree(g_ext2_fs.superblock);
        return -1;
    }

    // 块组描述符表从块2开始
    for (uint32_t i = 0; i < desc_table_blocks; i++) {
        if (ext2_read_block(2 + i, block_buffer + i * g_ext2_fs.block_size) < 0) {
            kfree(block_buffer);
            kfree(g_ext2_fs.group_descs);
            kfree(g_ext2_fs.superblock);
            return -1;
        }
    }

    memcpy(g_ext2_fs.group_descs, block_buffer, desc_table_size);
    kfree(block_buffer);

    g_ext2_initialized = 1;
    return 0;
}

int ext2_read_inode(uint32_t inode_num, ext2_inode_t *inode) {
    if (!g_ext2_initialized || !inode) return -1;

    uint32_t group = ext2_get_group_num(inode_num);
    uint32_t index = ext2_get_inode_index(inode_num);

    if (group >= g_ext2_fs.groups_count) return -1;

    // 计算inode所在的块
    uint32_t inode_table_block = g_ext2_fs.group_descs[group].inode_table;
    uint32_t block_offset = (index * g_ext2_fs.inode_size) / g_ext2_fs.block_size;
    uint32_t byte_offset = (index * g_ext2_fs.inode_size) % g_ext2_fs.block_size;

    uint8_t *buffer = (uint8_t *)kmalloc(g_ext2_fs.block_size);
    if (!buffer) return -1;

    if (ext2_read_block(inode_table_block + block_offset, buffer) < 0) {
        kfree(buffer);
        return -1;
    }

    memcpy(inode, buffer + byte_offset, sizeof(ext2_inode_t));
    kfree(buffer);

    return 0;
}

int ext2_write_inode(uint32_t inode_num, ext2_inode_t *inode) {
    if (!g_ext2_initialized || !inode) return -1;

    uint32_t group = ext2_get_group_num(inode_num);
    uint32_t index = ext2_get_inode_index(inode_num);

    if (group >= g_ext2_fs.groups_count) return -1;

    uint32_t inode_table_block = g_ext2_fs.group_descs[group].inode_table;
    uint32_t block_offset = (index * g_ext2_fs.inode_size) / g_ext2_fs.block_size;
    uint32_t byte_offset = (index * g_ext2_fs.inode_size) % g_ext2_fs.block_size;

    uint8_t *buffer = (uint8_t *)kmalloc(g_ext2_fs.block_size);
    if (!buffer) return -1;

    if (ext2_read_block(inode_table_block + block_offset, buffer) < 0) {
        kfree(buffer);
        return -1;
    }

    memcpy(buffer + byte_offset, inode, sizeof(ext2_inode_t));

    if (ext2_write_block(inode_table_block + block_offset, buffer) < 0) {
        kfree(buffer);
        return -1;
    }

    kfree(buffer);
    return 0;
}

/**
 * 获取inode的第block_index个块号
 */
static uint32_t ext2_get_block_num(ext2_inode_t *inode, uint32_t block_index) {
    uint8_t *indirect_buffer = (uint8_t *)kmalloc(g_ext2_fs.block_size);
    if (!indirect_buffer) return 0;

    uint32_t block_num = 0;
    uint32_t blocks_per_indirect = g_ext2_fs.block_size / sizeof(uint32_t);

    if (block_index < EXT2_DIRECT_BLOCKS) {
        // 直接块
        block_num = inode->block[block_index];
    } else if (block_index < EXT2_DIRECT_BLOCKS + blocks_per_indirect) {
        // 一级间接块
        uint32_t indirect_block = inode->block[EXT2_INDIRECT_BLOCK];
        if (indirect_block == 0) {
            kfree(indirect_buffer);
            return 0;
        }

        if (ext2_read_block(indirect_block, indirect_buffer) < 0) {
            kfree(indirect_buffer);
            return 0;
        }

        uint32_t index = block_index - EXT2_DIRECT_BLOCKS;
        uint32_t *block_ptrs = (uint32_t *)indirect_buffer;
        block_num = block_ptrs[index];
    } else if (block_index < EXT2_DIRECT_BLOCKS + blocks_per_indirect + blocks_per_indirect * blocks_per_indirect) {
        // 二级间接块
        uint32_t double_indirect_block = inode->block[EXT2_DOUBLE_INDIRECT_BLOCK];
        if (double_indirect_block == 0) {
            kfree(indirect_buffer);
            return 0;
        }

        uint32_t index = block_index - EXT2_DIRECT_BLOCKS - blocks_per_indirect;
        uint32_t first_level_index = index / blocks_per_indirect;
        uint32_t second_level_index = index % blocks_per_indirect;

        if (ext2_read_block(double_indirect_block, indirect_buffer) < 0) {
            kfree(indirect_buffer);
            return 0;
        }

        uint32_t *block_ptrs = (uint32_t *)indirect_buffer;
        uint32_t indirect_block = block_ptrs[first_level_index];

        if (indirect_block == 0) {
            kfree(indirect_buffer);
            return 0;
        }

        if (ext2_read_block(indirect_block, indirect_buffer) < 0) {
            kfree(indirect_buffer);
            return 0;
        }

        block_ptrs = (uint32_t *)indirect_buffer;
        block_num = block_ptrs[second_level_index];
    }

    kfree(indirect_buffer);
    return block_num;
}

int ext2_read_file(uint32_t inode_num, uint32_t offset, uint8_t *buffer, uint32_t size) {
    if (!g_ext2_initialized || !buffer) return -1;

    ext2_inode_t inode;
    if (ext2_read_inode(inode_num, &inode) < 0) return -1;

    if (offset >= inode.size) return 0;

    uint32_t read_size = (offset + size > inode.size) ? (inode.size - offset) : size;
    uint32_t bytes_read = 0;

    uint8_t *block_buffer = (uint8_t *)kmalloc(g_ext2_fs.block_size);
    if (!block_buffer) return -1;

    uint32_t current_offset = offset;
    uint32_t block_index = current_offset / g_ext2_fs.block_size;
    uint32_t block_offset = current_offset % g_ext2_fs.block_size;

    while (bytes_read < read_size) {
        uint32_t block_num = ext2_get_block_num(&inode, block_index);

        if (block_num == 0) break;

        if (ext2_read_block(block_num, block_buffer) < 0) {
            kfree(block_buffer);
            return -1;
        }

        uint32_t copy_size = g_ext2_fs.block_size - block_offset;
        if (copy_size > read_size - bytes_read) {
            copy_size = read_size - bytes_read;
        }

        memcpy(buffer + bytes_read, block_buffer + block_offset, copy_size);
        bytes_read += copy_size;
        block_offset = 0;
        block_index++;
    }

    kfree(block_buffer);
    return bytes_read;
}

/**
 * 设置inode的第block_index个块号
 */
static int ext2_set_block_num(ext2_inode_t *inode, uint32_t block_index, uint32_t block_num) {
    uint8_t *indirect_buffer = (uint8_t *)kmalloc(g_ext2_fs.block_size);
    if (!indirect_buffer) return -1;

    uint32_t blocks_per_indirect = g_ext2_fs.block_size / sizeof(uint32_t);
    int result = 0;

    if (block_index < EXT2_DIRECT_BLOCKS) {
        // 直接块
        inode->block[block_index] = block_num;
    } else if (block_index < EXT2_DIRECT_BLOCKS + blocks_per_indirect) {
        // 一级间接块
        uint32_t indirect_block = inode->block[EXT2_INDIRECT_BLOCK];
        if (indirect_block == 0) {
            indirect_block = ext2_allocate_block();
            if (indirect_block < 0) {
                kfree(indirect_buffer);
                return -1;
            }
            inode->block[EXT2_INDIRECT_BLOCK] = indirect_block;
            memset(indirect_buffer, 0, g_ext2_fs.block_size);
        } else {
            if (ext2_read_block(indirect_block, indirect_buffer) < 0) {
                kfree(indirect_buffer);
                return -1;
            }
        }

        uint32_t index = block_index - EXT2_DIRECT_BLOCKS;
        uint32_t *block_ptrs = (uint32_t *)indirect_buffer;
        block_ptrs[index] = block_num;

        if (ext2_write_block(indirect_block, indirect_buffer) < 0) {
            result = -1;
        }
    } else if (block_index < EXT2_DIRECT_BLOCKS + blocks_per_indirect + blocks_per_indirect * blocks_per_indirect) {
        // 二级间接块
        uint32_t double_indirect_block = inode->block[EXT2_DOUBLE_INDIRECT_BLOCK];
        if (double_indirect_block == 0) {
            double_indirect_block = ext2_allocate_block();
            if (double_indirect_block < 0) {
                kfree(indirect_buffer);
                return -1;
            }
            inode->block[EXT2_DOUBLE_INDIRECT_BLOCK] = double_indirect_block;
            memset(indirect_buffer, 0, g_ext2_fs.block_size);
            ext2_write_block(double_indirect_block, indirect_buffer);
        }

        uint32_t index = block_index - EXT2_DIRECT_BLOCKS - blocks_per_indirect;
        uint32_t first_level_index = index / blocks_per_indirect;
        uint32_t second_level_index = index % blocks_per_indirect;

        if (ext2_read_block(double_indirect_block, indirect_buffer) < 0) {
            kfree(indirect_buffer);
            return -1;
        }

        uint32_t *block_ptrs = (uint32_t *)indirect_buffer;
        uint32_t indirect_block = block_ptrs[first_level_index];

        if (indirect_block == 0) {
            indirect_block = ext2_allocate_block();
            if (indirect_block < 0) {
                kfree(indirect_buffer);
                return -1;
            }
            block_ptrs[first_level_index] = indirect_block;
            ext2_write_block(double_indirect_block, indirect_buffer);
            memset(indirect_buffer, 0, g_ext2_fs.block_size);
        } else {
            if (ext2_read_block(indirect_block, indirect_buffer) < 0) {
                kfree(indirect_buffer);
                return -1;
            }
        }

        block_ptrs = (uint32_t *)indirect_buffer;
        block_ptrs[second_level_index] = block_num;

        if (ext2_write_block(indirect_block, indirect_buffer) < 0) {
            result = -1;
        }
    }

    kfree(indirect_buffer);
    return result;
}

int ext2_write_file(uint32_t inode_num, uint32_t offset, uint8_t *buffer, uint32_t size) {
    if (!g_ext2_initialized || !buffer) return -1;

    ext2_inode_t inode;
    if (ext2_read_inode(inode_num, &inode) < 0) return -1;

    uint32_t bytes_written = 0;
    uint8_t *block_buffer = (uint8_t *)kmalloc(g_ext2_fs.block_size);
    if (!block_buffer) return -1;

    uint32_t current_offset = offset;
    uint32_t block_index = current_offset / g_ext2_fs.block_size;
    uint32_t block_offset = current_offset % g_ext2_fs.block_size;

    while (bytes_written < size) {
        uint32_t block_num = ext2_get_block_num(&inode, block_index);

        if (block_num == 0) {
            block_num = ext2_allocate_block();
            if (block_num < 0) {
                kfree(block_buffer);
                return -1;
            }
            if (ext2_set_block_num(&inode, block_index, block_num) < 0) {
                kfree(block_buffer);
                return -1;
            }
        }

        // 如果不是块的开始，需要先读取现有数据
        if (block_offset > 0 || (size - bytes_written < g_ext2_fs.block_size - block_offset)) {
            if (ext2_read_block(block_num, block_buffer) < 0) {
                kfree(block_buffer);
                return -1;
            }
        } else {
            memset(block_buffer, 0, g_ext2_fs.block_size);
        }

        uint32_t copy_size = g_ext2_fs.block_size - block_offset;
        if (copy_size > size - bytes_written) {
            copy_size = size - bytes_written;
        }

        memcpy(block_buffer + block_offset, buffer + bytes_written, copy_size);

        if (ext2_write_block(block_num, block_buffer) < 0) {
            kfree(block_buffer);
            return -1;
        }

        bytes_written += copy_size;
        block_offset = 0;
        block_index++;
    }

    // 更新inode大小和块数
    if (offset + bytes_written > inode.size) {
        inode.size = offset + bytes_written;
    }
    inode.blocks = (inode.size + 511) / 512;  // 以512字节块计算
    inode.mtime = 0;  // 应该设置为当前时间
    ext2_write_inode(inode_num, &inode);

    kfree(block_buffer);
    return bytes_written;
}

int ext2_list_dir(uint32_t inode_num, ext2_dir_entry_t *entries, uint32_t max_entries) {
    if (!g_ext2_initialized || !entries) return -1;

    ext2_inode_t inode;
    if (ext2_read_inode(inode_num, &inode) < 0) return -1;

    if (!(inode.mode & EXT2_S_IFDIR)) return -1;

    uint8_t *buffer = (uint8_t *)kmalloc(inode.size);
    if (!buffer) return -1;

    if (ext2_read_file(inode_num, 0, buffer, inode.size) < 0) {
        kfree(buffer);
        return -1;
    }

    uint32_t entry_count = 0;
    uint32_t offset = 0;

    while (offset < inode.size && entry_count < max_entries) {
        ext2_dir_entry_t *dir_entry = (ext2_dir_entry_t *)(buffer + offset);

        if (dir_entry->inode != 0) {
            entries[entry_count].inode = dir_entry->inode;
            entries[entry_count].rec_len = dir_entry->rec_len;
            entries[entry_count].name_len = dir_entry->name_len;
            entries[entry_count].file_type = dir_entry->file_type;
            memcpy(entries[entry_count].name, dir_entry->name, dir_entry->name_len);
            entries[entry_count].name[dir_entry->name_len] = '\0';
            entry_count++;
        }

        offset += dir_entry->rec_len;
    }

    kfree(buffer);
    return entry_count;
}

int ext2_find_entry(uint32_t parent_inode, const char *name, ext2_dir_entry_t *entry) {
    if (!g_ext2_initialized || !name || !entry) return -1;

    ext2_inode_t inode;
    if (ext2_read_inode(parent_inode, &inode) < 0) return -1;

    if (!(inode.mode & EXT2_S_IFDIR)) return -1;

    uint8_t *buffer = (uint8_t *)kmalloc(inode.size);
    if (!buffer) return -1;

    if (ext2_read_file(parent_inode, 0, buffer, inode.size) < 0) {
        kfree(buffer);
        return -1;
    }

    uint32_t offset = 0;
    uint32_t name_len = 0;
    while (name[name_len]) name_len++;

    while (offset < inode.size) {
        ext2_dir_entry_t *dir_entry = (ext2_dir_entry_t *)(buffer + offset);

        if (dir_entry->inode != 0 && dir_entry->name_len == name_len) {
            if (memcmp(dir_entry->name, name, name_len) == 0) {
                memcpy(entry, dir_entry, sizeof(ext2_dir_entry_t));
                kfree(buffer);
                return 0;
            }
        }

        offset += dir_entry->rec_len;
    }

    kfree(buffer);
    return -1;
}

int ext2_create_file(uint32_t parent_inode, const char *name, uint16_t mode) {
    if (!g_ext2_initialized || !name) return -1;

    // 分配新inode
    int new_inode_num = ext2_allocate_inode();
    if (new_inode_num < 0) return -1;

    // 初始化inode
    ext2_inode_t new_inode = {0};
    new_inode.mode = mode | EXT2_S_IFREG;
    new_inode.uid = 0;
    new_inode.size = 0;
    new_inode.atime = 0;
    new_inode.ctime = 0;
    new_inode.mtime = 0;
    new_inode.dtime = 0;
    new_inode.gid = 0;
    new_inode.links_count = 1;
    new_inode.blocks = 0;

    if (ext2_write_inode(new_inode_num, &new_inode) < 0) return -1;

    // 添加目录项
    ext2_inode_t parent;
    if (ext2_read_inode(parent_inode, &parent) < 0) return -1;

    uint32_t name_len = 0;
    while (name[name_len]) name_len++;

    uint32_t entry_size = sizeof(ext2_dir_entry_t) + name_len;
    entry_size = (entry_size + 3) & ~3;  // 对齐到4字节

    uint8_t *entry_buffer = (uint8_t *)kmalloc(entry_size);
    if (!entry_buffer) return -1;

    ext2_dir_entry_t *new_entry = (ext2_dir_entry_t *)entry_buffer;
    new_entry->inode = new_inode_num;
    new_entry->rec_len = entry_size;
    new_entry->name_len = name_len;
    new_entry->file_type = 1;  // 普通文件
    memcpy(new_entry->name, name, name_len);

    ext2_write_file(parent_inode, parent.size, entry_buffer, entry_size);
    kfree(entry_buffer);

    return new_inode_num;
}

int ext2_create_dir(uint32_t parent_inode, const char *name) {
    if (!g_ext2_initialized || !name) return -1;

    // 分配新inode
    int new_inode_num = ext2_allocate_inode();
    if (new_inode_num < 0) return -1;

    // 分配块
    int block_num = ext2_allocate_block();
    if (block_num < 0) return -1;

    // 初始化inode
    ext2_inode_t new_inode = {0};
    new_inode.mode = 0755 | EXT2_S_IFDIR;
    new_inode.uid = 0;
    new_inode.size = 0;
    new_inode.atime = 0;
    new_inode.ctime = 0;
    new_inode.mtime = 0;
    new_inode.dtime = 0;
    new_inode.gid = 0;
    new_inode.links_count = 2;
    new_inode.blocks = 2;
    new_inode.block[0] = block_num;

    if (ext2_write_inode(new_inode_num, &new_inode) < 0) return -1;

    // 添加到父目录
    ext2_inode_t parent;
    if (ext2_read_inode(parent_inode, &parent) < 0) return -1;

    uint32_t name_len = 0;
    while (name[name_len]) name_len++;

    uint32_t entry_size = sizeof(ext2_dir_entry_t) + name_len;
    entry_size = (entry_size + 3) & ~3;

    uint8_t *entry_buffer = (uint8_t *)kmalloc(entry_size);
    if (!entry_buffer) return -1;

    ext2_dir_entry_t *new_entry = (ext2_dir_entry_t *)entry_buffer;
    new_entry->inode = new_inode_num;
    new_entry->rec_len = entry_size;
    new_entry->name_len = name_len;
    new_entry->file_type = 2;  // 目录
    memcpy(new_entry->name, name, name_len);

    ext2_write_file(parent_inode, parent.size, entry_buffer, entry_size);
    kfree(entry_buffer);

    return new_inode_num;
}

int ext2_delete(uint32_t parent_inode, const char *name) {
    if (!g_ext2_initialized || !name) return -1;

    ext2_dir_entry_t entry;
    if (ext2_find_entry(parent_inode, name, &entry) < 0) return -1;

    // 简化版：仅标记inode为删除
    ext2_inode_t inode;
    if (ext2_read_inode(entry.inode, &inode) < 0) return -1;

    inode.dtime = 0;  // 应该设置为当前时间
    inode.links_count--;

    if (ext2_write_inode(entry.inode, &inode) < 0) return -1;

    return 0;
}

ext2_fs_t* ext2_get_fs(void) {
    if (!g_ext2_initialized) return NULL;
    return &g_ext2_fs;
}

void ext2_print_info(void) {
    if (!g_ext2_initialized) {
        //printf("EXT2 filesystem not initialized\n");
        return;
    }

    //printf("=== EXT2 Filesystem Info ===\n");
    //printf("Block size: %u bytes\n", g_ext2_fs.block_size);
    //printf("Inode size: %u bytes\n", g_ext2_fs.inode_size);
    //printf("Total blocks: %u\n", g_ext2_fs.superblock->blocks_count);
    //printf("Free blocks: %u\n", g_ext2_fs.superblock->free_blocks_count);
    //printf("Total inodes: %u\n", g_ext2_fs.superblock->inodes_count);
    //printf("Free inodes: %u\n", g_ext2_fs.superblock->free_inodes_count);
    //printf("Block groups: %u\n", g_ext2_fs.groups_count);
    //printf("Blocks per group: %u\n", g_ext2_fs.blocks_per_group);
    //printf("Inodes per group: %u\n", g_ext2_fs.inodes_per_group);
}
