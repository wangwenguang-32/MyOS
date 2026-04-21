#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t present;
    uint32_t sectors;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors_per_track;
} disk_info_t;

void disk_init(void);

int disk_read(uint32_t lba, uint8_t* buffer);

int disk_write(uint32_t lba, uint8_t* buffer);

disk_info_t* disk_get_info(void);

int disk_format(uint32_t start_lba, uint32_t sectors);

int disk_read_multiple(uint32_t lba, uint8_t* buffer, uint32_t sectors);

int disk_write_multiple(uint32_t lba, uint8_t* buffer, uint32_t sectors);

#define DISK_SECTOR_SIZE 512

#ifdef __cplusplus
}
#endif

#endif
