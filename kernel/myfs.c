#include "myfs.h"
#include "virtio.h"
#include <stdint.h>

extern void print(const char *str);
extern void print_uint(uint32_t n);

static myfs_super_t sb;
static int mounted = 0;

static int name_eq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}
static void name_copy(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static void read_inode(uint32_t i, myfs_inode_t *out) {
    uint8_t buf[512];
    uint32_t per_sector = 512 / sizeof(myfs_inode_t);
    uint32_t sec = sb.inode_start + i / per_sector;
    uint32_t off = (i % per_sector) * sizeof(myfs_inode_t);
    virtio_read_sector(sec, buf);
    for (uint32_t k = 0; k < sizeof(myfs_inode_t); k++)
        ((uint8_t *)out)[k] = buf[off + k];
}
static void write_inode(uint32_t i, const myfs_inode_t *in) {
    uint8_t buf[512];
    uint32_t per_sector = 512 / sizeof(myfs_inode_t);
    uint32_t sec = sb.inode_start + i / per_sector;
    uint32_t off = (i % per_sector) * sizeof(myfs_inode_t);
    virtio_read_sector(sec, buf);
    for (uint32_t k = 0; k < sizeof(myfs_inode_t); k++)
        buf[off + k] = ((const uint8_t *)in)[k];
    virtio_write_sector(sec, buf);
}

int myfs_is_mounted(void) { return mounted; }
uint32_t myfs_total(void) { return sb.total_sectors; }
uint32_t myfs_used(void) { return sb.data_start + MYFS_MAX_FILES; }
uint32_t myfs_free(void) { return sb.total_sectors > myfs_used() ? sb.total_sectors - myfs_used() : 0; }

int myfs_format(void) {
    uint32_t total = virtio_total_sectors();
    if (total == 0) return -1;

    sb.magic = MYFS_MAGIC;
    sb.version = 2;
    sb.total_sectors = total;
    sb.inode_count = MYFS_MAX_FILES;
    sb.inode_start = 1;
    sb.data_start = 1 + (MYFS_MAX_FILES * sizeof(myfs_inode_t) + 511) / 512;

    uint8_t buf[512];
    for (int i = 0; i < 512; i++) buf[i] = 0;
    for (uint32_t k = 0; k < sizeof(myfs_super_t); k++)
        buf[k] = ((uint8_t *)&sb)[k];
    virtio_write_sector(0, buf);

    myfs_inode_t empty;
    for (int i = 0; i < MYFS_NAME_MAX; i++) empty.name[i] = 0;
    empty.size = 0;
    for (int i = 0; i < MYFS_MAX_BLOCKS; i++) empty.blocks[i] = 0;
    empty.used = 0;
    for (uint32_t i = 0; i < MYFS_MAX_FILES; i++) write_inode(i, &empty);

    mounted = 1;
    return 0;
}

int myfs_mount(void) {
    uint8_t buf[512];
    virtio_read_sector(0, buf);
    for (uint32_t k = 0; k < sizeof(myfs_super_t); k++)
        ((uint8_t *)&sb)[k] = buf[k];
    if (sb.magic != MYFS_MAGIC) { mounted = 0; return -1; }
    mounted = 1;
    return 0;
}

int myfs_list(void (*cb)(const char *name, uint32_t size)) {
    if (!mounted) return -1;
    myfs_inode_t in;
    for (uint32_t i = 0; i < sb.inode_count; i++) {
        read_inode(i, &in);
        if (in.used && in.name[0]) cb(in.name, in.size);
    }
    return 0;
}

static int find_inode(const char *name, uint32_t *idx, myfs_inode_t *out) {
    myfs_inode_t in;
    for (uint32_t i = 0; i < sb.inode_count; i++) {
        read_inode(i, &in);
        if (in.used && name_eq(in.name, name)) {
            if (idx) *idx = i;
            if (out) *out = in;
            return 0;
        }
    }
    return -1;
}
static int alloc_inode(uint32_t *idx) {
    myfs_inode_t in;
    for (uint32_t i = 0; i < sb.inode_count; i++) {
        read_inode(i, &in);
        if (!in.used) { *idx = i; return 0; }
    }
    return -1;
}
static uint32_t alloc_block(void) {
    /* Ищем максимальный используемый LBA + 1 */
    myfs_inode_t in;
    uint32_t max_lba = sb.data_start - 1;
    for (uint32_t i = 0; i < sb.inode_count; i++) {
        read_inode(i, &in);
        if (!in.used) continue;
        for (int b = 0; b < MYFS_MAX_BLOCKS; b++) {
            if (in.blocks[b] > max_lba) max_lba = in.blocks[b];
        }
    }
    uint32_t next = max_lba + 1;
    if (next >= sb.total_sectors) return 0;
    return next;
}

int myfs_read(const char *name, char *buf, uint32_t max) {
    if (!mounted) return -1;
    myfs_inode_t in;
    if (find_inode(name, 0, &in) < 0) return -1;

    uint32_t n = in.size < max ? in.size : max;
    uint32_t done = 0;
    uint8_t tmp[512];
    for (int b = 0; b < MYFS_MAX_BLOCKS && done < n; b++) {
        if (in.blocks[b] == 0) break;
        virtio_read_sector(in.blocks[b], tmp);
        uint32_t chunk = n - done;
        if (chunk > 512) chunk = 512;
        for (uint32_t k = 0; k < chunk; k++) buf[done + k] = tmp[k];
        done += chunk;
    }
    return (int)done;
}

int myfs_write(const char *name, const char *data, uint32_t len) {
    if (!mounted) return -1;
    uint32_t max_bytes = MYFS_MAX_BLOCKS * MYFS_BLOCK_SIZE;
    if (len > max_bytes) len = max_bytes;

    myfs_inode_t in;
    uint32_t idx;
    int is_new = 0;
    if (find_inode(name, &idx, &in) < 0) {
        if (alloc_inode(&idx) < 0) return -1;
        in.used = 1;
        name_copy(in.name, name, MYFS_NAME_MAX);
        for (int b = 0; b < MYFS_MAX_BLOCKS; b++) in.blocks[b] = 0;
        is_new = 1;
    }

    /* Сколько блоков нужно */
    uint32_t need_blocks = (len + 511) / 512;
    if (need_blocks == 0) need_blocks = 1;

    /* Выделяем недостающие блоки */
    for (uint32_t b = 0; b < need_blocks; b++) {
        if (in.blocks[b] == 0) {
            uint32_t nb = alloc_block();
            if (nb == 0) {
                if (is_new) in.used = 0;
                return -1;
            }
            in.blocks[b] = nb;
        }
    }
    /* Освобождаем лишние (если файл стал короче) */
    for (uint32_t b = need_blocks; b < MYFS_MAX_BLOCKS; b++) {
        in.blocks[b] = 0;
    }

    in.size = len;

    /* Пишем данные по блокам */
    uint8_t tmp[512];
    uint32_t done = 0;
    for (uint32_t b = 0; b < need_blocks; b++) {
        for (int k = 0; k < 512; k++) tmp[k] = 0;
        uint32_t chunk = len - done;
        if (chunk > 512) chunk = 512;
        for (uint32_t k = 0; k < chunk; k++) tmp[k] = data[done + k];
        virtio_write_sector(in.blocks[b], tmp);
        done += chunk;
    }

    write_inode(idx, &in);
    return (int)len;
}

int myfs_delete(const char *name) {
    if (!mounted) return -1;
    myfs_inode_t in;
    uint32_t idx;
    if (find_inode(name, &idx, &in) < 0) return -1;
    in.used = 0;
    in.size = 0;
    for (int b = 0; b < MYFS_MAX_BLOCKS; b++) in.blocks[b] = 0;
    in.name[0] = 0;
    write_inode(idx, &in);
    return 0;
}

int myfs_append(const char *name, const char *data, uint32_t len) {
    if (!mounted) return -1;
    myfs_inode_t in;
    uint32_t idx;

    if (find_inode(name, &idx, &in) < 0) {
        /* файла нет — создаём как обычно */
        return myfs_write(name, data, len);
    }

    uint32_t old_size = in.size;
    uint32_t new_size = old_size + len;
    uint32_t max_bytes = MYFS_MAX_BLOCKS * MYFS_BLOCK_SIZE;
    if (new_size > max_bytes) new_size = max_bytes;
    len = new_size - old_size;
    if (len == 0) return 0;

    uint32_t need_blocks = (new_size + 511) / 512;
    if (need_blocks == 0) need_blocks = 1;

    /* выделить недостающие блоки */
    for (uint32_t b = 0; b < need_blocks; b++) {
        if (in.blocks[b] == 0) {
            uint32_t nb = alloc_block();
            if (nb == 0) return -1;
            in.blocks[b] = nb;
        }
    }

    /* дописать данные начиная с позиции old_size */
    uint8_t tmp[512];
    uint32_t written = 0;
    while (written < len) {
        uint32_t pos = old_size + written;
        uint32_t b = pos / 512;
        uint32_t off = pos % 512;
        uint32_t chunk = 512 - off;
        if (chunk > len - written) chunk = len - written;

        virtio_read_sector(in.blocks[b], tmp);
        for (uint32_t k = 0; k < chunk; k++) tmp[off + k] = data[written + k];
        virtio_write_sector(in.blocks[b], tmp);

        written += chunk;
    }

    in.size = new_size;
    write_inode(idx, &in);
    return (int)new_size;
}
