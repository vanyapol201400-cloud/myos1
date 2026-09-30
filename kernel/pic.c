#include <stdint.h>
#include "pic.h"
#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_CMD PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_CMD PIC2
#define PIC2_DATA (PIC2+1)
static inline void outb(uint16_t port, uint8_t val) { __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); }
void pic_init(void) {
    outb(PIC1_CMD,0x11); outb(PIC2_CMD,0x11);
    outb(PIC1_DATA,0x20); outb(PIC2_DATA,0x28);
    outb(PIC1_DATA,0x04); outb(PIC2_DATA,0x02);
    outb(PIC1_DATA,0x01); outb(PIC2_DATA,0x01);
    outb(PIC1_DATA,0xFC); outb(PIC2_DATA,0xFF);
}
void pic_eoi(int irq) { if (irq >= 8) outb(PIC2_CMD,0x20); outb(PIC1_CMD,0x20); }
