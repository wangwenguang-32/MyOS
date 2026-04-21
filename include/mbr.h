#ifndef MBR_H
#define MBR_H

#include <stdint.h>


// MBR 分区表项（单个分区，16 字节）
typedef struct {
    uint8_t  boot_flag;      // 0x80=可引导, 0x00=不可引导
    uint8_t  start_head;     // 起始磁头号
    uint8_t  start_sector;   // 起始扇区号（低6位）+ 起始柱面号高2位
    uint8_t  start_cylinder; // 起始柱面号低8位
    uint8_t  part_type;      // 分区类型（0x07=NTFS,0x83=Linux,0x0C=FAT32）
    uint8_t  end_head;       // 结束磁头号
    uint8_t  end_sector;     // 结束扇区号（低6位）+ 结束柱面号高2位
    uint8_t  end_cylinder;   // 结束柱面号低8位
    uint32_t lba_start;      // 起始扇区 LBA 地址（关键）
    uint32_t lba_size;       // 分区总扇区数
}__attribute__((packed)) mbr_partition_t;

// MBR 主结构体（总大小 512 字节）
typedef struct {
    uint8_t  boot_code[440]; // 引导加载程序代码（执行的汇编）
    uint32_t disk_signature; // 磁盘唯一签名（可选）
    uint16_t reserved;       // 保留字段（0x0000）
    mbr_partition_t partitions[4]; // 4 个主分区表项（4×16=64字节）
    uint16_t boot_signature; // 结束标志：必须 = 0xAA55
}__attribute__((packed)) mbr_t;

typedef struct {
    uint8_t     index;          // 分区号
    uint8_t     bootable;       // 可引导
    uint64_t    start_lba;      // 起始扇区
    uint64_t    sector_count;   // 总扇区数
    uint64_t    size_bytes;     // 大小
    uint8_t     type;           // 分区类型
    char        fs_name[16];    // 文件系统名
} part_t;

#define CHS_TO_LBA(C, H, S, HPC, SPT) \
    ((uint32_t)(((C) * (HPC) + (H)) * (SPT) + ((S) - 1)))


#endif