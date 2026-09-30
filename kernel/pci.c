#include "pci.h"

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off) {
    uint32_t addr = (1u << 31)
                  | ((uint32_t)bus << 16)
                  | ((uint32_t)slot << 11)
                  | ((uint32_t)func << 8)
                  | (off & 0xFC);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

int pci_find(uint16_t vendor, uint16_t device, uint8_t *bus, uint8_t *slot, uint8_t *func) {
    for (uint16_t b = 0; b < 256; b++) {
        for (uint8_t s = 0; s < 32; s++) {
            uint32_t v = pci_read(b, s, 0, 0);
            if (v == 0xFFFFFFFF || v == 0) continue;
            if ((v & 0xFFFF) == vendor && ((v >> 16) & 0xFFFF) == device) {
                *bus = b; *slot = s; *func = 0;
                return 0;
            }
        }
    }
    return -1;
}

void pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint32_t val) {
    uint32_t addr = (1u << 31)
                  | ((uint32_t)bus << 16)
                  | ((uint32_t)slot << 11)
                  | ((uint32_t)func << 8)
                  | (off & 0xFC);
    __asm__ volatile ("outl %0, %1" : : "a"(addr), "Nd"((uint16_t)0xCF8));
    __asm__ volatile ("outl %0, %1" : : "a"(val),  "Nd"((uint16_t)0xCFC));
}


extern void print(const char *str);
extern void print_uint(uint32_t n);
extern void print_hex8(uint8_t n);

void pci_scan_virtio(void) {
    print("\nPCI scan for virtio (1AF4:*):\n");
    int found = 0;
    for (uint16_t b = 0; b < 256; b++) {
        for (uint8_t s = 0; s < 32; s++) {
            uint32_t v = pci_read(b, s, 0, 0);
            if (v == 0xFFFFFFFF || v == 0) continue;
            if ((v & 0xFFFF) == 0x1AF4) {
                print("  bus="); print_uint(b);
                print(" slot="); print_uint(s);
                print(" device=0x");
                print_hex8((v >> 16) & 0xFF);
                print_hex8((v >> 24) & 0xFF);
                print("\n");
                found = 1;
            }
        }
    }
    if (!found) print("  no virtio devices found\n");
}

extern void print(const char *str);
extern void print_uint(uint32_t n);
extern void print_hex8(uint8_t n);


extern void print(const char *str);
extern void print_uint(uint32_t n);
extern void print_hex8(uint8_t n);

void pci_scan_all_1af4(void) {
    print("\nAll 1AF4 devices:\n");
    for (uint16_t b = 0; b < 256; b++) {
        for (uint8_t s = 0; s < 32; s++) {
            uint32_t v = pci_read(b, s, 0, 0);
            if (v == 0xFFFFFFFF || v == 0) continue;
            if ((v & 0xFFFF) == 0x1AF4) {
                uint16_t dev = (v >> 16) & 0xFFFF;
                print("  bus="); print_uint(b);
                print(" slot="); print_uint(s);
                print(" dev=0x"); 
                print_hex8((dev >> 8) & 0xFF); print_hex8(dev & 0xFF);
                if (dev == 0x1000) print(" (virtio-net legacy)");
                else if (dev == 0x1001) print(" (virtio-blk legacy)");
                else if (dev == 0x1041) print(" (virtio-net modern)");
                else if (dev == 0x1042) print(" (virtio-blk modern)");
                print("\n");

                /* BAR0 = I/O base для legacy */
                uint32_t bar0 = pci_read(b, s, 0, 0x10);
                print("    BAR0=0x");
                for (int i = 28; i >= 0; i -= 4) print_hex8((bar0 >> i) & 0xF);
                print("\n");
            }
        }
    }
}
