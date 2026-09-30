#ifndef ATA_H
#define ATA_H
#include <stdint.h>
int      ata_init(void);
void     ata_read_sector(uint32_t lba, uint8_t *buf);
void     ata_write_sector(uint32_t lba, const uint8_t *buf);
uint32_t ata_total_sectors(void);
void     ata_drive_info(char *model_out, uint32_t *sectors_out);
#endif
