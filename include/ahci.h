#ifndef AHCI_H
#define AHCI_H

#include<stdint.h>
#include<pci.h>


/* 磁盘类型 */
typedef enum {
	PORT_NONE = 0,
	PORT_HDD  = 1,
	PORT_SSD  = 2,
} PortType;

/* 单个 SATA 端口（每个硬盘一个） */
typedef struct AhciPort {
	PortType type;
	char     serial[21];    /* 序列号 */
	char     model[41];     /* 型号 */
	char     firmware[9];   /* 固件版本 */
	uint64_t total_sectors; /* 总扇区数 */
	uint32_t sector_size;   /* 扇区大小 512 */
	uint64_t capacity;      /* 总容量 byte */
} AhciPort;

/* AHCI 设备结构体 */
typedef struct AhciController 
{
	PCIAddress addr;

	uint32_t  mmio_pa;  /* 物理地址 */
	uint32_t  mmio_size;

	/*所有 SATA 端口 */
	uint8_t  port_cnt;
	AhciPort ports[32];

	/* 驱动用到的命令缓冲区 */
	uintptr_t cmdlist_va;
	uint32_t  cmdlist_pa;
	uintptr_t cmdtbl_va;
	uint32_t  cmdtbl_pa;
} AhciController;
typedef AhciController ahci_controller_t;


extern ahci_controller_t ahci;

int ahci_init(void);


int ahci_read_sector(uint64_t lba, uint32_t buf_pa);
int ahci_read_sectors(uint64_t lba, uint32_t sector_count, uint32_t buf_pa);
int ahci_write_sector(uint64_t lba, uint32_t buf_pa);
int ahci_write_sectors(uint64_t lba, uint32_t sector_count, uint32_t buf_pa);
int ahci_identify_disk(uint32_t buf_pa);
uint64_t ahci_get_disk_size(void);
int ahci_is_disk_ready(void);
uint32_t ahci_get_disk_status(void);

// ========================
// 错误码定义
// ========================
#define AHCI_ERR_MMIO_NOT_SET    -1
#define AHCI_ERR_PORT_NOT_IMPL   -2
#define AHCI_ERR_LINK_NOT_READY  -3
#define AHCI_ERR_NOT_SATA_DISK   -4
#define AHCI_ERR_STOP_PORT_FAIL  -5
#define AHCI_ERR_PORT_BUSY       -10
#define AHCI_ERR_CMD_FAILED      -11
#define AHCI_ERR_CMD_TIMEOUT     -12
#define AHCI_ERR_CMD_ERROR       -13
#define AHCI_ERR_INVALID_SECTORS -15

#endif // __AHCI_H__
