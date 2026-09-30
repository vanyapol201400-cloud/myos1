#ifndef VIRTIO_H
#define VIRTIO_H
#include <stdint.h>

int      virtio_init(void);
void     virtio_read_sector(uint32_t lba, uint8_t *buf);
void     virtio_write_sector(uint32_t lba, const uint8_t *buf);
uint32_t virtio_total_sectors(void);
void     virtio_drive_info(char *model_out, uint32_t *sectors_out);

#endif
