#include <stdint.h>
#include "mouse.h"

#define VGA 0xB8000
#define W 80

static int last_x = -1, last_y = -1;
static char saved_ch = ' ';
static uint8_t saved_attr = 0x0F;

void mouse_cursor_draw(void) {
    int x = mouse_get_x();
    int y = mouse_get_y();
    if (x == last_x && y == last_y) return;

    volatile char *vga = (volatile char *)VGA;
    if (last_x >= 0) {
        int p = last_y * W + last_x;
        vga[p * 2] = saved_ch;
        vga[p * 2 + 1] = saved_attr;
    }
    int p = y * W + x;
    saved_ch = vga[p * 2];
    saved_attr = vga[p * 2 + 1];
    vga[p * 2] = 0xDB;
    vga[p * 2 + 1] = 0x70;
    last_x = x;
    last_y = y;
}

void mouse_cursor_hide(void) {
    if (last_x >= 0) {
        volatile char *vga = (volatile char *)VGA;
        int p = last_y * W + last_x;
        vga[p * 2] = saved_ch;
        vga[p * 2 + 1] = saved_attr;
        last_x = -1;
    }
}
