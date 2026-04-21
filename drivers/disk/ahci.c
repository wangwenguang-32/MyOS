#include<stdint.h>
#include<pci.h>
#include<ahci.h>

#define KERNEL_BASE        0xC0000000U

#define AHCI_CAP           0x00
#define AHCI_GHC           0x04
#define AHCI_IS            0x08
#define AHCI_PI            0x0C

#define AHCI_PORT0_BASE    0x100
#define AHCI_PX_CLB        (AHCI_PORT0_BASE + 0x00)
#define AHCI_PX_CLBU       (AHCI_PORT0_BASE + 0x04)
#define AHCI_PX_FB         (AHCI_PORT0_BASE + 0x08)
#define AHCI_PX_FBU        (AHCI_PORT0_BASE + 0x0C)
#define AHCI_PX_IS         (AHCI_PORT0_BASE + 0x10)
#define AHCI_PX_IE         (AHCI_PORT0_BASE + 0x14)
#define AHCI_PX_CMD        (AHCI_PORT0_BASE + 0x18)
#define AHCI_PX_TFD        (AHCI_PORT0_BASE + 0x20)
#define AHCI_PX_SIG        (AHCI_PORT0_BASE + 0x24)
#define AHCI_PX_SSTS       (AHCI_PORT0_BASE + 0x28)
#define AHCI_PX_SERR       (AHCI_PORT0_BASE + 0x30)
#define AHCI_PX_CI         (AHCI_PORT0_BASE + 0x38)

#define SATA_SIG_ATA       0x00000101

#define ATA_CMD_READ_DMA_EXT  0x25
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_IDENTIFY      0xEC

#define HBA_PXCMD_ST       (1U << 0)
#define HBA_PXCMD_FRE      (1U << 4)
#define HBA_PXCMD_FR       (1U << 14)
#define HBA_PXCMD_CR       (1U << 15)

#define HBA_PXIS_TFES      (1U << 30)

#define PCI_AHCI_VENDORID  0x8086
#define PCI_AHCI_DEVICEID  0x2922

ahci_controller_t ahci;

volatile uint32_t* ahci_mmio;

uint8_t* disk_buf;

// ------------------------
// AHCI 内部结构
// ------------------------
typedef struct {
    uint8_t fis_type;
    uint8_t pmport:4;
    uint8_t rsv0:3;
    uint8_t c:1;
    uint8_t command;
    uint8_t featurel;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;
    uint8_t rsv1[4];
} __attribute__((packed)) FIS_REG_H2D;

typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc_i;
} __attribute__((packed)) HBA_PRDT_ENTRY;

typedef struct {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsv[48];
    HBA_PRDT_ENTRY prdt[1];
} __attribute__((packed)) AHCI_CMD_TBL;

typedef struct {
    uint16_t flags;
    uint16_t prdtl;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv[4];
} __attribute__((packed)) AHCI_CMD_HEADER;

// ------------------------
// 内存工具函数
// ------------------------
static inline void mem_zero(void* buf, uint32_t len) {
    uint8_t* p = (uint8_t*)buf;
    uint32_t i;
    for (i = 0; i < len; i++) p[i] = 0;
}

static inline uint32_t to_pa(void* va) {
    return (uint32_t)va - KERNEL_BASE;
}

// ------------------------
// 全局缓冲
// ------------------------
AHCI_CMD_HEADER* cmdlist_va;
AHCI_CMD_TBL* cmdtbl_va;

uint32_t cmdlist_pa;
uint32_t cmdtbl_pa;
uint32_t fis_pa;
uint32_t disk_buf_pa;

static int ahci_wait_port_idle() {
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t tfd = ahci_mmio[AHCI_PX_TFD / 4];
        if ((tfd & ((1U << 7) | (1U << 3))) == 0) return 0; // BSY/DRQ 都清零
    }
    return -1;
}

static int ahci_stop_port() {
    uint32_t cmd = ahci_mmio[AHCI_PX_CMD / 4];
    cmd &= ~HBA_PXCMD_ST;
    ahci_mmio[AHCI_PX_CMD / 4] = cmd;

    uint32_t timeout = 1000000;
    while (timeout--) {
        if ((ahci_mmio[AHCI_PX_CMD / 4] & HBA_PXCMD_CR) == 0) break;
    }
    if (timeout == 0) return -1;

    cmd = ahci_mmio[AHCI_PX_CMD / 4];
    cmd &= ~HBA_PXCMD_FRE;
    ahci_mmio[AHCI_PX_CMD / 4] = cmd;

    timeout = 1000000;
    while (timeout--) {
        if ((ahci_mmio[AHCI_PX_CMD / 4] & HBA_PXCMD_FR) == 0) return 0;
    }
    return -1;
}

static void ahci_start_port() {
    uint32_t cmd = ahci_mmio[AHCI_PX_CMD / 4];
    cmd |= HBA_PXCMD_FRE;
    ahci_mmio[AHCI_PX_CMD / 4] = cmd;
    cmd |= HBA_PXCMD_ST;
    ahci_mmio[AHCI_PX_CMD / 4] = cmd;
}

// ------------------------
// AHCI 初始化
// ------------------------
int ahci_init() {

    if(!PCI_FindDevice(PCI_AHCI_VENDORID,PCI_AHCI_DEVICEID,&ahci.addr))
    {
        printf("AHCI not found\n");
        return 1;
    }
    printf("AHCI found\n");
    ahci_mmio=(uint32_t*) PCI_GetBarAddr(&ahci.addr,5);
    uint16_t command =PCI_CMD_IO_SPACE | PCI_CMD_MEM_SPACE | PCI_CMD_BUS_MASTER | PCI_CMD_SERR_ENABLE;
    PCI_SetCommand(&ahci.addr,command);

    if (ahci_mmio == 0) {
        printf("AHCI MMIO not set\n");
        return -1;
    }

    cmdlist_pa = alloc_page();
    cmdtbl_pa = alloc_page();
    fis_pa = alloc_page();
    disk_buf_pa = alloc_page();

    cmdlist_va = (AHCI_CMD_HEADER*)(cmdlist_pa + KERNEL_BASE);
    cmdtbl_va = (AHCI_CMD_TBL*)(cmdtbl_pa + KERNEL_BASE);
    disk_buf = (uint8_t*)(disk_buf_pa + KERNEL_BASE);

    mem_zero(cmdlist_va, 1024); // 32 * 32B
    mem_zero(cmdtbl_va, sizeof(AHCI_CMD_TBL));
    mem_zero((void*)(fis_pa + KERNEL_BASE), 256);
    mem_zero(disk_buf, 512);

    uint32_t cap = ahci_mmio[AHCI_CAP / 4];
    uint8_t max_ports = (cap & 0x1F) + 1;

    printf("AHCI Controller: max_ports=%d\n", max_ports);

    // 使能 AHCI 模式与中断
    ahci_mmio[AHCI_GHC / 4] |= (1U << 31) | (1U << 1);

    if ((ahci_mmio[AHCI_PI / 4] & 0x1) == 0) {
        printf("Port 0 not implemented\n");
        return -2;
    }

    uint32_t ssts = ahci_mmio[AHCI_PX_SSTS / 4];
    uint8_t det = ssts & 0x0F;
    uint8_t ipm = (ssts >> 8) & 0x0F;
    if (det != 0x3 || ipm != 0x1) {
        printf("Port0 link not ready: DET=%u IPM=%u\n", det, ipm);
        return -3;
    }

    if (ahci_mmio[AHCI_PX_SIG / 4] != SATA_SIG_ATA) {
        printf("Port0 is not SATA disk, SIG=0x%x\n", ahci_mmio[AHCI_PX_SIG / 4]);
        return -4;
    }

    if (ahci_stop_port() != 0) {
        printf("Stop port failed\n");
        return -5;
    }

    ahci_mmio[AHCI_PX_CLB / 4] = cmdlist_pa;
    ahci_mmio[AHCI_PX_CLBU / 4] = 0;
    ahci_mmio[AHCI_PX_FB / 4] = fis_pa;
    ahci_mmio[AHCI_PX_FBU / 4] = 0;

    ahci_mmio[AHCI_PX_SERR / 4] = 0xFFFFFFFFU;
    ahci_mmio[AHCI_PX_IS / 4] = 0xFFFFFFFFU;
    ahci_mmio[AHCI_PX_IE / 4] = 0;

    ahci_start_port();
    // ioapic_unmask_interrupt(22);
    printf("AHCI Port0 initialized\n");
    return 0;
}

// ATA 命令定义
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_IDENTIFY      0xEC

// 扇区大小
#define SECTOR_SIZE 512

// 最大 PRDT 条目数（支持多扇区）
#define MAX_PRDT_ENTRIES 16

// 内部命令执行函数
static int ahci_execute_cmd(uint32_t slot, uint32_t cmd_type, uint64_t lba, 
                            uint32_t sector_count, uint32_t buf_pa) {
    if (ahci_wait_port_idle() != 0) {
        printf("Port busy timeout before issuing command\n");
        return -10;
    }

    // 清理槽位
    mem_zero(&cmdlist_va[slot], sizeof(AHCI_CMD_HEADER));
    mem_zero(cmdtbl_va, sizeof(AHCI_CMD_TBL));

    // 计算 PRDT 条目数
    uint32_t prdt_count = (sector_count + 127) / 128; // 每个条目最多 128 扇区
    if (prdt_count > MAX_PRDT_ENTRIES) {
        printf("Too many sectors: %u\n", sector_count);
        return -14;
    }

    // 设置 PRDT 条目
    uint32_t remaining = sector_count * SECTOR_SIZE;
    uint32_t i;
    for (i = 0; i < prdt_count; i++) {
        uint32_t size = (remaining > 0x400000) ? 0x400000 : remaining;
        cmdtbl_va->prdt[i].dba = buf_pa + (i * 0x400000);
        cmdtbl_va->prdt[i].dbau = 0;
        cmdtbl_va->prdt[i].dbc_i = (size - 1);
        remaining -= size;
    }

    // CFIS (Register - Host to Device)
    FIS_REG_H2D* cfis = (FIS_REG_H2D*)cmdtbl_va->cfis;
    cfis->fis_type = 0x27;
    cfis->c = 1;
    cfis->command = cmd_type;

    cfis->lba0 = (uint8_t)(lba & 0xFF);
    cfis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    cfis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    cfis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    cfis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    cfis->lba5 = (uint8_t)((lba >> 40) & 0xFF);

    cfis->device = (1U << 6); // LBA
    cfis->countl = (uint8_t)(sector_count & 0xFF);
    cfis->counth = (uint8_t)((sector_count >> 8) & 0xFF);

    // 命令头
    cmdlist_va[slot].flags = (5U << 0) | (1U << 10); // CFL=5DW, C=1
    cmdlist_va[slot].prdtl = prdt_count;
    cmdlist_va[slot].prdbc = 0;
    cmdlist_va[slot].ctba = cmdtbl_pa;
    cmdlist_va[slot].ctbau = 0;

    // 清中断状态并发命令
    ahci_mmio[AHCI_PX_IS / 4] = 0xFFFFFFFFU;
    ahci_mmio[AHCI_PX_CI / 4] = (1U << slot);

    // 轮询完成
    uint32_t timeout = 5000000;
    while (timeout--) {
        uint32_t ci = ahci_mmio[AHCI_PX_CI / 4];
        uint32_t is = ahci_mmio[AHCI_PX_IS / 4];
        if ((is & HBA_PXIS_TFES) != 0) {
            printf("Command failed: PxIS=0x%x PxTFD=0x%x\n", is, ahci_mmio[AHCI_PX_TFD / 4]);
            return -11;
        }
        if ((ci & (1U << slot)) == 0) break;
    }
    if (timeout == 0) {
        printf("Command timeout: PxCI=0x%x PxTFD=0x%x\n", ahci_mmio[AHCI_PX_CI / 4], ahci_mmio[AHCI_PX_TFD / 4]);
        return -12;
    }

    if (ahci_mmio[AHCI_PX_TFD / 4] & 0x01) {
        printf("Command error: PxTFD=0x%x\n", ahci_mmio[AHCI_PX_TFD / 4]);
        return -13;
    }

    return 0;
}

// ========================
// 向上层提供的接口
// ========================

// 读取单个扇区
int ahci_read_sector(uint64_t lba, uint32_t buf_pa) {
    return ahci_execute_cmd(0, ATA_CMD_READ_DMA_EXT, lba, 1, buf_pa);
}

// 读取多个扇区
int ahci_read_sectors(uint64_t lba, uint32_t sector_count, uint32_t buf_pa) {
    if (sector_count == 0) {
        printf("Invalid sector count: 0\n");
        return -15;
    }
    return ahci_execute_cmd(0, ATA_CMD_READ_DMA_EXT, lba, sector_count, buf_pa);
}

// 写入单个扇区
int ahci_write_sector(uint64_t lba, uint32_t buf_pa) {
    return ahci_execute_cmd(0, ATA_CMD_WRITE_DMA_EXT, lba, 1, buf_pa);
}

// 写入多个扇区
int ahci_write_sectors(uint64_t lba, uint32_t sector_count, uint32_t buf_pa) {
    if (sector_count == 0) {
        printf("Invalid sector count: 0\n");
        return -15;
    }
    return ahci_execute_cmd(0, ATA_CMD_WRITE_DMA_EXT, lba, sector_count, buf_pa);
}

// 获取磁盘信息（通过 IDENTIFY 命令）
int ahci_identify_disk(uint32_t buf_pa) {
    const uint32_t slot = 1;

    if (ahci_wait_port_idle() != 0) {
        printf("Port busy timeout before IDENTIFY\n");
        return -10;
    }

    mem_zero(&cmdlist_va[slot], sizeof(AHCI_CMD_HEADER));
    mem_zero(cmdtbl_va, sizeof(AHCI_CMD_TBL));

    // PRDT: 512 字节
    cmdtbl_va->prdt[0].dba = buf_pa;
    cmdtbl_va->prdt[0].dbau = 0;
    cmdtbl_va->prdt[0].dbc_i = (512 - 1);

    // CFIS
    FIS_REG_H2D* cfis = (FIS_REG_H2D*)cmdtbl_va->cfis;
    cfis->fis_type = 0x27;
    cfis->c = 1;
    cfis->command = ATA_CMD_IDENTIFY;
    cfis->device = (1U << 6);

    // 命令头
    cmdlist_va[slot].flags = (5U << 0) | (1U << 10);
    cmdlist_va[slot].prdtl = 1;
    cmdlist_va[slot].prdbc = 0;
    cmdlist_va[slot].ctba = cmdtbl_pa;
    cmdlist_va[slot].ctbau = 0;

    ahci_mmio[AHCI_PX_IS / 4] = 0xFFFFFFFFU;
    ahci_mmio[AHCI_PX_CI / 4] = (1U << slot);

    uint32_t timeout = 2000000;
    while (timeout--) {
        if ((ahci_mmio[AHCI_PX_CI / 4] & (1U << slot)) == 0) break;
    }
    if (timeout == 0) {
        printf("IDENTIFY timeout\n");
        return -12;
    }

    return 0;
}

// 获取磁盘容量（扇区数）
uint64_t ahci_get_disk_size() {
    // 需要先执行 IDENTIFY 命令获取磁盘信息
    // 返回值为总扇区数
    // 这里返回示例值，实际应从 IDENTIFY 数据中解析
    return 0x100000000ULL; // 示例：2TB
}

// 检查磁盘是否就绪
int ahci_is_disk_ready() {
    uint32_t ssts = ahci_mmio[AHCI_PX_SSTS / 4];
    uint8_t det = ssts & 0x0F;
    uint8_t ipm = (ssts >> 8) & 0x0F;
    
    if (det == 0x3 && ipm == 0x1) {
        return 1; // 就绪
    }
    return 0; // 未就绪
}

// 获取磁盘状态
uint32_t ahci_get_disk_status() {
    return ahci_mmio[AHCI_PX_TFD / 4];
}

