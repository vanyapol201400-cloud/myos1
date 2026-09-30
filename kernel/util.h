#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
uint8_t  cmos_read(uint8_t reg);
void     rtc_read(uint8_t *h, uint8_t *m, uint8_t *s,
                  uint8_t *day, uint8_t *mon, uint16_t *year);
void     hex_print(uint32_t n);
void     hex_dump(uint32_t addr, int bytes);
#endif
uint8_t cmos_get(uint8_t reg);
