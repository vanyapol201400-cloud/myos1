#include "virtio.h"
#include "pci.h"
#include "io.h"
#include <stdint.h>

extern void print(const char *str);
extern void print_uint(uint32_t n);
extern void print_hex32(uint32_t v);
extern void print_hex8(uint8_t n);

/* Legacy virtio 0.9 PCI, I/O BAR0 */
static uint16_t io_base = 0;
static uint32_t capacity = 0;

/* Legacy virtio register offsets (from I/O BAR) */
#define VIRTIO_PCI_HOST_FEATURES    0x00
#define VIRTIO_PCI_GUEST_FEATURES   0x04
#define VIRTIO_PCI_QUEUE_PFN        0x08
#define VIRTIO_PCI_QUEUE_NUM        0x0C
#define VIRTIO_PCI_QUEUE_SEL        0x0E
#define VIRTIO_PCI_QUEUE_NOTIFY     0x10
#define VIRTIO_PCI_STATUS           0x12
#define VIRTIO_PCI_ISR              0x13
#define VIRTIO_PCI_CONFIG           0x14

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER      2
#define VIRTIO_STATUS_DRIVER_OK   4
#define VIRTIO_STATUS_FEATURES_OK 8

/* Virtqueue (legacy: должен быть выровнен на 4096, одна страница) */
#define QSIZE 128
#define VQ_PAGE 0x1400000     /* физический адрес страницы очереди */

struct vq_desc { uint64_t addr; uint32_t len; uint16_t flags; uint16_t next; } __attribute__((packed));
struct vq_avail { uint16_t flags; uint16_t idx; uint16_t ring[QSIZE]; uint16_t used_event; } __attribute__((packed));
struct vq_used_elem { uint32_t id; uint32_t len; } __attribute__((packed));
struct vq_used { uint16_t flags; uint16_t idx; struct vq_used_elem ring[QSIZE]; uint16_t avail_event; } __attribute__((packed));

static struct vq_desc  *desc  = (struct vq_desc  *)VQ_PAGE;
static struct vq_avail *avail = (struct vq_avail *)(VQ_PAGE + 0x1000);
static struct vq_used  *used  = (struct vq_used  *)(VQ_PAGE + 0x2000);

struct virtio_blk_req { uint32_t type; uint32_t reserved; uint64_t sector; } __attribute__((packed));
static struct virtio_blk_req *req_hdr = (struct virtio_blk_req *)(VQ_PAGE + 0x4000);
static uint8_t *req_status = (uint8_t *)(VQ_PAGE + 0x5000);
static uint8_t *data_buf   = (uint8_t *)(VQ_PAGE + 0x6000);

static uint16_t last_used_idx = 0;
static uint16_t avail_idx = 0;

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint32_t inl(uint16_t port) {
    uint32_t v;
    __asm__ volatile ("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

int virtio_init(void) {
    uint8_t b, s, f;
    print("\nvirtio-blk legacy init...\n");

    if (pci_find(0x1AF4, 0x1001, &b, &s, &f) != 0) {
        print("virtio-blk not found\n");
        return -1;
    }

    uint32_t cmd = pci_read(b, s, f, 0x04);
    if (!(cmd & 0x06)) pci_write(b, s, f, 0x04, cmd | 0x06);

    uint32_t bar0 = pci_read(b, s, f, 0x10);
    io_base = (uint16_t)(bar0 & 0xFFFC);
    print("io_base = 0x"); print_hex32(io_base); print("\n");

    /* Reset */
    outb(io_base + VIRTIO_PCI_STATUS, 0);
    for (volatile int i = 0; i < 1000000; i++) { }

    /* ACKNOWLEDGE */
    outb(io_base + VIRTIO_PCI_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    /* DRIVER */
    outb(io_base + VIRTIO_PCI_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

    uint32_t status = inb(io_base + VIRTIO_PCI_STATUS);
    print("status = 0x"); print_hex8((uint8_t)status); print("\n");

    /* Host features */
    uint32_t host_features = inl(io_base + VIRTIO_PCI_HOST_FEATURES);
    print("host_features = 0x"); print_hex32(host_features); print("\n");

    /* Guest features = 0 (минимум) */
    outl(io_base + VIRTIO_PCI_GUEST_FEATURES, 0);

    /* Queue 0 */
    outw(io_base + VIRTIO_PCI_QUEUE_SEL, 0);
    uint32_t qnum = inw(io_base + VIRTIO_PCI_QUEUE_NUM);
    print("queue_num = "); print_uint(qnum); print("\n");
    if (qnum == 0 || qnum > QSIZE) qnum = QSIZE;

    /* Обнулить очередь */
    for (int i = 0; i < QSIZE; i++) {
        desc[i].addr = 0; desc[i].len = 0; desc[i].flags = 0; desc[i].next = 0;
        avail->ring[i] = 0;
        used->ring[i].id = 0; used->ring[i].len = 0;
    }
    avail->flags = 0; avail->idx = 0;
    used->flags = 0; used->idx = 0;

    /* Legacy: пишем PFN (номер страницы) в QUEUE_PFN */
    uint32_t pfn = VQ_PAGE >> 12;
    outl(io_base + VIRTIO_PCI_QUEUE_PFN, pfn);
    print("queue pfn = "); print_uint(pfn); print("\n");

    /* DRIVER_OK */
    outb(io_base + VIRTIO_PCI_STATUS,
         VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK);

    /* Capacity из device config (первые 8 байт) */
    uint32_t cap_lo = inl(io_base + VIRTIO_PCI_CONFIG + 0);
    uint32_t cap_hi = inl(io_base + VIRTIO_PCI_CONFIG + 4);
    (void)cap_hi;
    capacity = cap_lo;

    print("capacity = "); print_uint(capacity); print(" sectors\n");
    print("virtio-blk OK\n");
    return 0;
}

static int virtio_rw(uint32_t type, uint32_t lba, uint8_t *buf) {
    for (int i = 0; i < 512; i++) data_buf[i] = buf[i];

    req_hdr->type = type;
    req_hdr->reserved = 0;
    req_hdr->sector = lba;
    *req_status = 0xFF;

    desc[0].addr  = (uint32_t)req_hdr;
    desc[0].len   = sizeof(struct virtio_blk_req);
    desc[0].flags = 1;
    desc[0].next  = 1;

    desc[1].addr  = (uint32_t)data_buf;
    desc[1].len   = 512;
    desc[1].flags = (type == 0) ? 3 : 1;
    desc[1].next  = 2;

    desc[2].addr  = (uint32_t)req_status;
    desc[2].len   = 1;
    desc[2].flags = 2;
    desc[2].next  = 0;

    avail->ring[avail_idx % QSIZE] = 0;
    avail_idx++;
    avail->idx = avail_idx;
    __asm__ volatile ("mfence" ::: "memory");

    /* Notify queue 0 */
    outw(io_base + VIRTIO_PCI_QUEUE_NOTIFY, 0);

    for (int i = 0; i < 100000000; i++) {
        if (used->idx != last_used_idx) {
            last_used_idx = used->idx;
            if (*req_status != 0) return -1;
            if (type == 0) for (int k = 0; k < 512; k++) buf[k] = data_buf[k];
            return 0;
        }
    }
    return -1;
}

void virtio_read_sector(uint32_t lba, uint8_t *buf)  { virtio_rw(0, lba, buf); }
void virtio_write_sector(uint32_t lba, const uint8_t *buf) { virtio_rw(1, lba, (uint8_t *)buf); }
uint32_t virtio_total_sectors(void) { return capacity; }
void virtio_drive_info(char *m, uint32_t *s) {
    const char *n = "virtio-blk";
    int i = 0; while (n[i]) { m[i] = n[i]; i++; } m[i] = 0;
    *s = capacity;
}
