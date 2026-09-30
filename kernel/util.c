#include "util.h"
#include "io.h"

extern void putchar(char c);
extern void print(const char *str);

uint8_t cmos_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_bin(uint8_t b) {
    return (b & 0x0F) + ((b >> 4) * 10);
}

void rtc_read(uint8_t *h, uint8_t *m, uint8_t *s,
              uint8_t *day, uint8_t *mon, uint16_t *year) {
    while (cmos_read(0x0A) & 0x80) { }   /* ждём, пока RTC не занят */
    *s   = bcd_to_bin(cmos_read(0x00));
    *m   = bcd_to_bin(cmos_read(0x02));
    *h   = bcd_to_bin(cmos_read(0x04));
    *day = bcd_to_bin(cmos_read(0x07));
    *mon = bcd_to_bin(cmos_read(0x08));
    *year = 2000 + bcd_to_bin(cmos_read(0x09));
}

void hex_print(uint32_t n) {
    const char *hex = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        putchar(hex[(n >> i) & 0xF]);
    }
}

void hex_dump(uint32_t addr, int bytes) {
    const char *hex = "0123456789ABCDEF";
    uint8_t *p = (uint8_t *)addr;
    for (int i = 0; i < bytes; i++) {
        putchar(hex[(p[i] >> 4) & 0xF]);
        putchar(hex[p[i] & 0xF]);
        putchar(' ');
    }
}

