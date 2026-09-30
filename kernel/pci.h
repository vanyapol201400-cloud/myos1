#ifndef PCI_H
#define PCI_H
#include <stdint.h>
uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off);
void pci_write(uint8_t b, uint8_t s, uint8_t f, uint8_t off, uint32_t val);
int pci_find(uint16_t vendor, uint16_t device, uint8_t *bus, uint8_t *slot, uint8_t *func);
#endif
void pci_scan_virtio(void);
void pci_scan_all_1af4(void);
