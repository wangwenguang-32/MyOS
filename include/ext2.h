#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================
// EXT2 常量定义
// ========================

#define EXT2_BLOCK_SIZE 4096
#define EXT2_INODE_SIZE 128
#define EXT2_MAX_FILENAME 255
#define EXT2_DIRECT_BLOCKS 12
#define EXT2_INDIRECT_BLOCK 13
#define EXT2_DOUBLE_INDIRECT_BLOCK 14
#define EXT2_TRIPLE_INDIRECT_BLOCK 15

// 文件类型
#define EXT2_S_IFREG  0x8000  // 普通文件
#define EXT2_S_IFDIR  0x4000  // 目录
#define EXT2_S_IFLNK  0xA000  // 符号链接

// ========================
// EXT2 超级块结构
// ========================

typedef struct {
    uint32_t inodes_count;           // inode总数
    uint32_t blocks_count;           // 块总数
    uint32_t reserved_blocks_count;  // 保留块数
    uint32_t free_blocks_count;      // 空闲块数
    uint32_t free_inodes_count;      // 空闲inode数
    uint32_t block_size;             // 块大小（1024 << block_size_log2）
    uint32_t fragment_size;          // 片段大小
    uint32_t blocks_per_group;       // 每组块数
    uint32_t fragments_per_group;    // 每组片段数
    uint32_t inodes_per_group;       // 每组inode数
    uint32_t mtime;                  // 最后挂载时间
    uint32_t wtime;                  // 最后写入时间
    uint16_t mnt_count;              // 挂载次数
    uint16_t max_mnt_count;          // 最大挂载次数
    uint16_t magic;                  // 魔数 (0xEF53)
    uint16_t state;                  // 文件系统状态
    uint16_t errors;                 // 错误处理方式
    uint16_t minor_revision_level;   // 次版本号
    uint32_t lastcheck;              // 最后检查时间
    uint32_t checkinterval;          // 检查间隔
    uint32_t creator_os;             // 创建者OS
    uint32_t revision_level;         // 主版本号
    uint16_t uid_reserved;           // 保留uid
    uint16_t gid_reserved;           // 保留gid
    uint32_t first_ino;              // 第一个非保留inode
    uint16_t inode_size;             // inode大小
    uint16_t block_group_nr;         // 块组号
    uint32_t feature_compat;         // 兼容特性
    uint32_t feature_incompat;       // 不兼容特性
    uint32_t feature_ro_compat;      // 只读兼容特性
    uint8_t uuid[16];                // UUID
    uint8_t volume_name[16];         // 卷名
    uint8_t last_mounted[64];        // 最后挂载路径
    uint32_t algo_usage_bitmap;      // 算法使用位图
    uint8_t reserved[820];           // 保留
} ext2_superblock_t;

// ========================
// EXT2 块组描述符
// ========================

typedef struct {
    uint32_t block_bitmap;           // 块位图块号
    uint32_t inode_bitmap;           // inode位图块号
    uint32_t inode_table;            // inode表块号
    uint16_t free_blocks_count;      // 空闲块数
    uint16_t free_inodes_count;      // 空闲inode数
    uint16_t used_dirs_count;        // 已用目录数
    uint16_t pad;                    // 填充
    uint8_t reserved[12];            // 保留
} ext2_group_desc_t;

// ========================
// EXT2 Inode 结构
// ========================

typedef struct {
    uint16_t mode;                   // 文件模式和权限
    uint16_t uid;                    // 用户ID
    uint32_t size;                   // 文件大小（字节）
    uint32_t atime;                  // 访问时间
    uint32_t ctime;                  // 创建时间
    uint32_t mtime;                  // 修改时间
    uint32_t dtime;                  // 删除时间
    uint16_t gid;                    // 组ID
    uint16_t links_count;            // 硬链接数
    uint32_t blocks;                 // 块数（512字节块）
    uint32_t flags;                  // 文件标志
    uint32_t osd1;                   // OS相关字段1
    uint32_t block[15];              // 块指针（12直接+3间接）
    uint32_t generation;             // 生成号
    uint32_t file_acl;               // 文件ACL块
    uint32_t dir_acl;                // 目录ACL块
    uint32_t faddr;                  // 片段地址
    uint8_t osd2[12];                // OS相关字段2
} ext2_inode_t;

// ========================
// EXT2 目录项结构
// ========================

typedef struct {
    uint32_t inode;                  // inode号
    uint16_t rec_len;                // 目录项长度
    uint8_t name_len;                // 文件名长度
    uint8_t file_type;               // 文件类型
    char name[EXT2_MAX_FILENAME];    // 文件名
} ext2_dir_entry_t;

// ========================
// EXT2 文件系统结构
// ========================

typedef struct {
    uint32_t block_size;             // 块大小
    uint32_t inode_size;             // inode大小
    uint32_t blocks_per_group;       // 每组块数
    uint32_t inodes_per_group;       // 每组inode数
    uint32_t groups_count;           // 块组数
    ext2_superblock_t *superblock;   // 超级块
    ext2_group_desc_t *group_descs;  // 块组描述符表
} ext2_fs_t;

// ========================
// EXT2 API
// ========================

/**
 * 初始化EXT2文件系统
 * @param start_lba 文件系统起始LBA
 * @return 0成功，负数表示错误
 */
int ext2_init(uint32_t start_lba);

/**
 * 读取inode
 * @param inode_num inode号
 * @param inode 输出缓冲区
 * @return 0成功，负数表示错误
 */
int ext2_read_inode(uint32_t inode_num, ext2_inode_t *inode);

/**
 * 写入inode
 * @param inode_num inode号
 * @param inode inode数据
 * @return 0成功，负数表示错误
 */
int ext2_write_inode(uint32_t inode_num, ext2_inode_t *inode);

/**
 * 读取文件内容
 * @param inode_num inode号
 * @param offset 文件偏移
 * @param buffer 输出缓冲区
 * @param size 要读取的字节数
 * @return 实际读取的字节数，负数表示错误
 */
int ext2_read_file(uint32_t inode_num, uint32_t offset, uint8_t *buffer, uint32_t size);

/**
 * 写入文件内容
 * @param inode_num inode号
 * @param offset 文件偏移
 * @param buffer 数据缓冲区
 * @param size 要写入的字节数
 * @return 实际写入的字节数，负数表示错误
 */
int ext2_write_file(uint32_t inode_num, uint32_t offset, uint8_t *buffer, uint32_t size);

/**
 * 列出目录内容
 * @param inode_num 目录inode号
 * @param entries 输出目录项数组
 * @param max_entries 最大目录项数
 * @return 实际目录项数，负数表示错误
 */
int ext2_list_dir(uint32_t inode_num, ext2_dir_entry_t *entries, uint32_t max_entries);

/**
 * 查找目录项
 * @param parent_inode 父目录inode号
 * @param name 文件名
 * @param entry 输出目录项
 * @return 0成功，负数表示错误
 */
int ext2_find_entry(uint32_t parent_inode, const char *name, ext2_dir_entry_t *entry);

/**
 * 创建文件
 * @param parent_inode 父目录inode号
 * @param name 文件名
 * @param mode 文件模式
 * @return 新inode号，负数表示错误
 */
int ext2_create_file(uint32_t parent_inode, const char *name, uint16_t mode);

/**
 * 创建目录
 * @param parent_inode 父目录inode号
 * @param name 目录名
 * @return 新inode号，负数表示错误
 */
int ext2_create_dir(uint32_t parent_inode, const char *name);

/**
 * 删除文件或目录
 * @param parent_inode 父目录inode号
 * @param name 文件/目录名
 * @return 0成功，负数表示错误
 */
int ext2_delete(uint32_t parent_inode, const char *name);

/**
 * 获取文件系统信息
 * @return 指向ext2_fs_t的指针
 */
ext2_fs_t* ext2_get_fs(void);

/**
 * 打印文件系统信息
 */
void ext2_print_info(void);

#ifdef __cplusplus
}
#endif

#endif
