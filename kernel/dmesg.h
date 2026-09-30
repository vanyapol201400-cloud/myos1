#ifndef DMESG_H
#define DMESG_H
#include <stdint.h>

#define DMESG_SIZE 8192

void     dmesg_init(void);
void     dmesg_putchar(char c);
void     dmesg_write(const char *s);
void     dmesg_write_uint(uint32_t n);
void     dmesg_write_hex(uint32_t n);
void     dmesg_clear(void);
int      dmesg_len(void);
const char *dmesg_buf(void);

#endif
