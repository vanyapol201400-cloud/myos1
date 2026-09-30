#include <stdint.h>
#include "mouse.h"

extern void print(const char *s);
extern void print_uint(uint32_t n);

void cmd_mouse_info(void) {
    print("\nmouse x="); print_uint(mouse_get_x());
    print(" y="); print_uint(mouse_get_y());
    print(" buttons="); print_uint(mouse_get_buttons());
}
