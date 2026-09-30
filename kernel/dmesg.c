#include "dmesg.h"

static char     buf[DMESG_SIZE];
static uint32_t len = 0;

void dmesg_init(void) { len = 0; }

void dmesg_putchar(char c) {
    if (len >= DMESG_SIZE - 1) {
        /* сдвигаем на 1 КБ влево */
        for (uint32_t i = 0; i < DMESG_SIZE - 1025; i++) {
            buf[i] = buf[i + 1024];
        }
        len -= 1024;
    }
    buf[len++] = c;
    buf[len] = 0;
}

void dmesg_write(const char *s) {
    while (*s) dmesg_putchar(*s++);
}

void dmesg_write_uint(uint32_t n) {
    if (n == 0) { dmesg_putchar('0'); return; }
    char tmp[16];
    int i = 0;
    while (n) { tmp[i++] = '0' + (n % 10); n /= 10; }
    while (i--) dmesg_putchar(tmp[i]);
}

void dmesg_write_hex(uint32_t n) {
    const char *h = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        dmesg_putchar(h[(n >> i) & 0xF]);
    }
}

void dmesg_clear(void) { len = 0; buf[0] = 0; }

int dmesg_len(void) { return (int)len; }
const char *dmesg_buf(void) { return buf; }
