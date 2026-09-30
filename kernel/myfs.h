#ifndef MYFS_H
#define MYFS_H
#include <stdint.h>

#define MYFS_MAGIC      0x4D594653
#define MYFS_NAME_MAX   28
#define MYFS_MAX_FILES  64
#define MYFS_BLOCK_SIZE 512
#define MYFS_MAX_BLOCKS 16      /* до 8 КБ на файл */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t total_sectors;
    uint32_t inode_count;
    uint32_t inode_start;
    uint32_t data_start;
} myfs_super_t;

typedef struct {
    char     name[MYFS_NAME_MAX];
    uint32_t size;
    uint32_t blocks[MYFS_MAX_BLOCKS];   /* LBA блоков данных, 0 = нет */
    uint32_t used;
} myfs_inode_t;

int  myfs_format(void);
int  myfs_mount(void);
int  myfs_is_mounted(void);

int  myfs_list(void (*cb)(const char *name, uint32_t size));
int  myfs_read(const char *name, char *buf, uint32_t max);
int  myfs_write(const char *name, const char *data, uint32_t len);
int  myfs_append(const char *name, const char *data, uint32_t len);
int  myfs_delete(const char *name);

uint32_t myfs_total(void);
uint32_t myfs_used(void);
uint32_t myfs_free(void);
#endif
