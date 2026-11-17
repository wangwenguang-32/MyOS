#include <mbr.h>
#include <block_device.h>
#include <stddef.h>

part_t mbr_part[MBR_PARTITION_COUNT];
static uint32_t partition_count;

typedef char mbr_partition_size_check[(sizeof(mbr_partition_t) == 16) ? 1 : -1];
typedef char mbr_sector_size_check[(sizeof(mbr_t) == BLK_SECTOR_SIZE) ? 1 : -1];
typedef char mbr_table_offset_check[(offsetof(mbr_t, partitions) == 446) ? 1 : -1];

static void reset_partitions(void)
{
    uint32_t i;
    partition_count = 0;
    for (i = 0; i < MBR_PARTITION_COUNT; i++) {
        mbr_part[i] = (part_t){0};
        mbr_part[i].index = i;
    }
}

/* These are partition type labels, not filesystem probe results. */
static const char *partition_type_name(uint8_t type)
{
    switch (type) {
    case 0x01: return "FAT12";
    case 0x04: case 0x06: case 0x0e: return "FAT16";
    case 0x0b: case 0x0c: return "FAT32";
    case 0x05: case 0x0f: case 0x85: return "Extended";
    case 0x07: return "NTFS/HPFS/exFAT";
    case 0x82: return "Linux swap";
    case 0x83: return "Linux";
    case 0x8e: return "Linux LVM";
    case 0xee: return "GPT protective";
    case 0xef: return "EFI system";
    default: return "Unknown";
    }
}

int mbr_init(void)
{
    mbr_t sector;
    part_t parsed[MBR_PARTITION_COUNT] = {0};
    const blk_info_t *device = blk_get_info();
    uint32_t i, j, count = 0;

    reset_partitions();
    if (!blk_is_initialized() || !device || !device->present || !device->sectors)
        return MBR_ERR_NO_DEVICE;
    /* Buffer I/O keeps partition parsing independent of cache internals. */
    if (blk_read(0, (uint8_t *)&sector, 1)) return MBR_ERR_IO;
    if (sector.boot_signature != MBR_SIGNATURE) return MBR_ERR_SIGNATURE;

    for (i = 0; i < MBR_PARTITION_COUNT; i++) {
        const mbr_partition_t *entry = &sector.partitions[i];
        part_t *part = &parsed[i];
        uint64_t end = (uint64_t)entry->lba_start + entry->lba_size;
        const char *name;
        part->index = i;
        /* Ignore stale CHS bytes in otherwise unused entries. */
        if (!entry->part_type && !entry->lba_start && !entry->lba_size &&
            !entry->boot_flag) continue;
        if (!entry->part_type || !entry->lba_start || !entry->lba_size ||
            (entry->boot_flag != 0 && entry->boot_flag != 0x80))
            return MBR_ERR_ENTRY;
        /* A protective entry may describe more than the block API exposes. */
        if (end > device->sectors && entry->part_type != MBR_TYPE_GPT_PROTECTIVE)
            return MBR_ERR_RANGE;
        if (entry->lba_start >= device->sectors) return MBR_ERR_RANGE;
        for (j = 0; j < i; j++) {
            const part_t *previous = &parsed[j];
            if (!previous->type) continue;
            /* Hybrid GPT/MBR layouts can overlap the protective entry. */
            if (entry->part_type == MBR_TYPE_GPT_PROTECTIVE ||
                previous->type == MBR_TYPE_GPT_PROTECTIVE) continue;
            if ((uint64_t)entry->lba_start < previous->start_lba + previous->sector_count &&
                previous->start_lba < end) return MBR_ERR_OVERLAP;
        }
        part->bootable = entry->boot_flag == 0x80;
        part->start_lba = entry->lba_start;
        part->sector_count = entry->lba_size;
        part->size_bytes = (uint64_t)entry->lba_size * BLK_SECTOR_SIZE;
        part->type = entry->part_type;
        name = partition_type_name(part->type);
        for (j = 0; j + 1 < sizeof(part->fs_name) && name[j]; j++)
            part->fs_name[j] = name[j];
        part->fs_name[j] = '\0';
        count++;
    }
    /* Publish only after the entire table passes validation. */
    for (i = 0; i < MBR_PARTITION_COUNT; i++) mbr_part[i] = parsed[i];
    partition_count = count;
    return 0;
}

uint32_t mbr_get_partition_count(void) { return partition_count; }

const part_t *mbr_get_partition(uint32_t index)
{
    if (index >= MBR_PARTITION_COUNT || !mbr_part[index].type) return NULL;
    return &mbr_part[index];
}
