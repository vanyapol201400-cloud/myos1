#include <stdint.h>
static inline void outb_font(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static const unsigned char dos866[4096] = {
    /* СЮДА ВСТАВЬ МАССИВ ИЗ СВОЕГО СООБЩЕНИЯ */
};
void load_ru_font(void) {
    volatile uint8_t *vga_font = (uint8_t *)0xA0000;
    outb_font(0x3C4, 0x02); outb_font(0x3C5, 0x04);
    outb_font(0x3C4, 0x04); outb_font(0x3C5, 0x02);
    outb_font(0x3CE, 0x04); outb_font(0x3CF, 0x00);
    outb_font(0x3CE, 0x05); outb_font(0x3CF, 0x00);
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 16; j++) {
            vga_font[i * 32 + j] = dos866[i * 16 + j];
        }
    }
    outb_font(0x3C4, 0x02); outb_font(0x3C5, 0x03);
    outb_font(0x3C4, 0x04); outb_font(0x3C5, 0x02);
    outb_font(0x3CE, 0x04); outb_font(0x3CF, 0x00);
    outb_font(0x3CE, 0x05); outb_font(0x3CF, 0x00);
}
