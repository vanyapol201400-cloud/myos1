#include "virtio_net.h"
#include "pci.h"
#include "io.h"
#include <stdint.h>

extern void print(const char *str);
extern void print_uint(uint32_t n);
extern void print_hex8(uint8_t n);

/* Legacy virtio-net PCI (1AF4:1000) */
#define VIRTIO_NET_VENDOR 0x1AF4
#define VIRTIO_NET_DEVICE 0x1000

/* Legacy virtio registers (от I/O base) */
#define VN_HOST_FEATURES    0x00
#define VN_GUEST_FEATURES   0x04
#define VN_QUEUE_PFN        0x08
#define VN_QUEUE_NUM        0x0C
#define VN_QUEUE_SEL        0x0E
#define VN_QUEUE_NOTIFY     0x10
#define VN_STATUS           0x12
#define VN_ISR              0x13
#define VN_MAC              0x14   /* 6 байт MAC-адреса (только для virtio-net) */

#define VN_STATUS_ACK   1
#define VN_STATUS_DRV   2
#define VN_STATUS_OK    4

#define QSIZE 128
#define VQ_PAGE 0x2000000    /* 32 МБ — для virtqueue */

struct vq_desc { uint64_t addr; uint32_t len; uint16_t flags; uint16_t next; } __attribute__((packed));
struct vq_avail { uint16_t flags; uint16_t idx; uint16_t ring[QSIZE]; uint16_t used_event; } __attribute__((packed));
struct vq_used_elem { uint32_t id; uint32_t len; } __attribute__((packed));
struct vq_used { uint16_t flags; uint16_t idx; struct vq_used_elem ring[QSIZE]; uint16_t avail_event; } __attribute__((packed));

/* Virtio-net header (10 байт для legacy без mergeable) */
struct virtio_net_hdr {
    uint8_t  flags;
    uint8_t  gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
} __attribute__((packed));

static uint16_t io_base = 0;
static uint8_t  mac[6];
static int      initialized = 0;

/* Приёмная очередь (0) и передающая (1) */
static struct vq_desc  *rx_desc  = (struct vq_desc  *)(VQ_PAGE);
static struct vq_avail *rx_avail = (struct vq_avail *)(VQ_PAGE + 0x1000);
static struct vq_used  *rx_used  = (struct vq_used  *)(VQ_PAGE + 0x2000);

static struct vq_desc  *tx_desc  = (struct vq_desc  *)(VQ_PAGE + 0x3000);
static struct vq_avail *tx_avail = (struct vq_avail *)(VQ_PAGE + 0x4000);
static struct vq_used  *tx_used  = (struct vq_used  *)(VQ_PAGE + 0x5000);

/* Приёмные буферы */
static uint8_t rx_bufs[16][2048] __attribute__((aligned(16)));
static struct virtio_net_hdr rx_hdrs[16];

/* Передающий буфер */
static uint8_t tx_buf[2048] __attribute__((aligned(16)));
static struct virtio_net_hdr tx_hdr;

static uint16_t tx_last_used = 0;
static uint16_t rx_last_used = 0;

static inline uint8_t inb_p(uint16_t port) { return inb(port); }
static inline void outb_p(uint16_t port, uint8_t v) { outb(port, v); }
static inline uint32_t inl_p(uint16_t port) {
    uint32_t v; __asm__ volatile ("inl %1, %0" : "=a"(v) : "Nd"(port)); return v;
}
static inline void outl_p(uint16_t port, uint32_t v) {
    __asm__ volatile ("outl %0, %1" : : "a"(v), "Nd"(port));
}

static void outw_p(uint16_t port, uint16_t v) {
    __asm__ volatile ("outw %0, %1" : : "a"(v), "Nd"(port));
}
static uint16_t inw_p(uint16_t port) {
    uint16_t v; __asm__ volatile ("inw %1, %0" : "=a"(v) : "Nd"(port)); return v;
}

int virtio_net_init(void) {
    uint8_t b, s, f;
    print("\nvirtio-net init...\n");

    if (pci_find(VIRTIO_NET_VENDOR, VIRTIO_NET_DEVICE, &b, &s, &f) != 0) {
        print("virtio-net not found\n");
        return -1;
    }

    uint32_t cmd = pci_read(b, s, f, 0x04);
    if (!(cmd & 0x06)) pci_write(b, s, f, 0x04, cmd | 0x06);

    uint32_t bar0 = pci_read(b, s, f, 0x10);
    io_base = (uint16_t)(bar0 & 0xFFFC);
    print("net io_base = 0x");
    print_hex8((io_base >> 8) & 0xFF); print_hex8(io_base & 0xFF);
    print("\n");

    /* Reset */
    outb_p(io_base + VN_STATUS, 0);
    for (volatile int i = 0; i < 1000000; i++) { }

    /* ACK + DRIVER */
    outb_p(io_base + VN_STATUS, VN_STATUS_ACK);
    outb_p(io_base + VN_STATUS, VN_STATUS_ACK | VN_STATUS_DRV);

    uint32_t host_features = inl_p(io_base + VN_HOST_FEATURES);
    print("net host_features = 0x");
    for (int i = 28; i >= 0; i -= 4) print_hex8((host_features >> i) & 0xF);
    print("\n");

    /* Guest features = 0 (минимум) */
    outl_p(io_base + VN_GUEST_FEATURES, 0);

    /* Прочитать MAC-адрес из device config */
    for (int i = 0; i < 6; i++) {
        mac[i] = inb_p(io_base + VN_MAC + i);
    }
    print("MAC: ");
    for (int i = 0; i < 6; i++) {
        print_hex8(mac[i]);
        if (i < 5) print(":");
    }
    print("\n");

    /* Обнулить очереди */
    for (int i = 0; i < QSIZE; i++) {
        rx_desc[i].addr = 0; rx_desc[i].len = 0; rx_desc[i].flags = 0; rx_desc[i].next = 0;
        tx_desc[i].addr = 0; tx_desc[i].len = 0; tx_desc[i].flags = 0; tx_desc[i].next = 0;
        rx_avail->ring[i] = 0;
        tx_avail->ring[i] = 0;
        rx_used->ring[i].id = 0; rx_used->ring[i].len = 0;
        tx_used->ring[i].id = 0; tx_used->ring[i].len = 0;
    }
    rx_avail->flags = 0; rx_avail->idx = 0;
    tx_avail->flags = 0; tx_avail->idx = 0;
    rx_used->flags = 0; rx_used->idx = 0;
    tx_used->flags = 0; tx_used->idx = 0;

    /* Настроить RX очередь (0) */
    outw_p(io_base + VN_QUEUE_SEL, 0);
    uint32_t qnum = inw_p(io_base + VN_QUEUE_NUM);
    if (qnum == 0 || qnum > 16) qnum = 16;

    for (int i = 0; i < 16 && i < (int)qnum; i++) {
        rx_desc[i].addr = (uint32_t)&rx_hdrs[i];
        rx_desc[i].len = sizeof(struct virtio_net_hdr);
        rx_desc[i].flags = 1;   /* NEXT */
        rx_desc[i].next = i + 16;

        rx_desc[i + 16].addr = (uint32_t)rx_bufs[i];
        rx_desc[i + 16].len = 2048;
        rx_desc[i + 16].flags = 2;   /* WRITE */
        rx_desc[i + 16].next = 0;

        rx_avail->ring[i] = i;
    }
    rx_avail->idx = 16;
    outl_p(io_base + VN_QUEUE_PFN, VQ_PAGE >> 12);

    /* Настроить TX очередь (1) */
    outw_p(io_base + VN_QUEUE_SEL, 1);
    outl_p(io_base + VN_QUEUE_PFN, (VQ_PAGE + 0x3000) >> 12);

    /* DRIVER_OK */
    outb_p(io_base + VN_STATUS, VN_STATUS_ACK | VN_STATUS_DRV | VN_STATUS_OK);

    initialized = 1;
    print("virtio-net OK\n");
    return 0;
}

void virtio_net_get_mac(uint8_t *out) {
    for (int i = 0; i < 6; i++) out[i] = mac[i];
}

int virtio_net_send(const uint8_t *data, uint32_t len) {
    if (!initialized) return -1;
    if (len > 1500) return -1;

    /* Заголовок */
    tx_hdr.flags = 0; tx_hdr.gso_type = 0; tx_hdr.hdr_len = 0;
    tx_hdr.gso_size = 0; tx_hdr.csum_start = 0; tx_hdr.csum_offset = 0;

    /* Копируем данные */
    for (uint32_t i = 0; i < len; i++) tx_buf[i] = data[i];

    /* Дескрипторы: заголовок + данные */
    tx_desc[0].addr = (uint32_t)&tx_hdr;
    tx_desc[0].len = sizeof(struct virtio_net_hdr);
    tx_desc[0].flags = 1;   /* NEXT */
    tx_desc[0].next = 1;

    tx_desc[1].addr = (uint32_t)tx_buf;
    tx_desc[1].len = len;
    tx_desc[1].flags = 0;
    tx_desc[1].next = 0;

    /* Добавить в avail ring */
    tx_avail->ring[tx_avail->idx % QSIZE] = 0;
    tx_avail->idx++;
    __asm__ volatile ("mfence" ::: "memory");

    /* Notify */
    outw_p(io_base + VN_QUEUE_NOTIFY, 1);

    /* Ждать used */
    for (int i = 0; i < 10000000; i++) {
        if (tx_used->idx != tx_last_used) {
            tx_last_used = tx_used->idx;
            return 0;
        }
    }
    return -1;
}

int virtio_net_recv(uint8_t *buf, uint32_t max) {
    if (!initialized) return 0;
    if (rx_used->idx == rx_last_used) return 0;

    /* Есть пакет */
    uint32_t idx = rx_used->ring[rx_last_used % QSIZE].id;
    uint32_t len = rx_used->ring[rx_last_used % QSIZE].len;
    rx_last_used++;

    /* len включает virtio_net_hdr (10 байт) */
    if (len > sizeof(struct virtio_net_hdr)) {
        uint32_t pkt_len = len - sizeof(struct virtio_net_hdr);
        if (pkt_len > max) pkt_len = max;
        uint8_t *src = rx_bufs[idx / 2];   /* idx = i или i+16, буфер = i */
        for (uint32_t i = 0; i < pkt_len; i++) buf[i] = src[i];
        return (int)pkt_len;
    }
    return 0;
}
