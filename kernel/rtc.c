#include <stdint.h>

/* RTC CMOS — прямой доступ через порты (inline asm, без io.h) */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

static int rtc_updating(void) {
    outb(CMOS_ADDR, 0x0A);
    return inb(CMOS_DATA) & 0x80;
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/* Читает время: hour, min, sec */
void rtc_read_time(uint8_t *h, uint8_t *m, uint8_t *s) {
    while (rtc_updating()) { }

    uint8_t sec  = cmos_read(0x00);
    uint8_t min  = cmos_read(0x02);
    uint8_t hour = cmos_read(0x04);
    uint8_t regB = cmos_read(0x0B);

    if (!(regB & 0x04)) {
        sec  = bcd_to_bin(sec);
        min  = bcd_to_bin(min);
        hour = bcd_to_bin(hour & 0x7F) | (hour & 0x80);
    }
    if (!(regB & 0x02)) {
        if (hour & 0x80) hour = ((hour & 0x7F) + 12) % 24;
    }

    *h = hour;
    *m = min;
    *s = sec;
}

/* Дата: day, month, year */
void rtc_read_date(uint8_t *d, uint8_t *mo, uint16_t *y) {
    while (rtc_updating()) { }

    uint8_t day   = cmos_read(0x07);
    uint8_t month = cmos_read(0x08);
    uint8_t year  = cmos_read(0x09);
    uint8_t regB  = cmos_read(0x0B);

    if (!(regB & 0x04)) {
        day   = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year  = bcd_to_bin(year);
    }
    *d  = day;
    *mo = month;
    *y  = 2000 + year;
}
