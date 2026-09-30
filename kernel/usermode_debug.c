#include <stdint.h>
extern void print(const char *s);
extern void print_hex32(uint32_t n);

void print_enter_info(uint32_t entry, uint32_t stack) {
    print("\n[enter: eip=0x"); print_hex32(entry);
    print(" esp=0x"); print_hex32(stack);
    print("]\n");
}
