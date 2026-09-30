#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H
#include <stdint.h>

int  virtio_net_init(void);
void virtio_net_get_mac(uint8_t *mac);
int  virtio_net_send(const uint8_t *data, uint32_t len);
int  virtio_net_recv(uint8_t *buf, uint32_t max);

#endif
