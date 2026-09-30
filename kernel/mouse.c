#include "mouse.h"
#include "io.h"
#include <stdint.h>

#define MOUSE_DATA   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_CMD    0x64
#define SCREEN_W 80
#define SCREEN_H 25

static uint8_t cycle = 0;
static uint8_t packet[4];
static int mx = 40, my = 12;
static int buttons = 0;
static int moved = 0;

static void wait_write(void) {
    for (int i = 0; i < 100000; i++)
        if (!(inb(MOUSE_STATUS) & 0x02)) return;
}
static void wait_read(void) {
    for (int i = 0; i < 100000; i++)
        if (inb(MOUSE_STATUS) & 0x01) return;
}
static void mouse_write(uint8_t v) {
    wait_write();
    outb(MOUSE_CMD, 0xD4);
    wait_write();
    outb(MOUSE_DATA, v);
    wait_read();
    inb(MOUSE_DATA);
}

void mouse_init(void) {
    wait_write();
    outb(MOUSE_CMD, 0xA8);

    wait_write();
    outb(MOUSE_CMD, 0x20);
    wait_read();
    uint8_t status = inb(MOUSE_DATA);
    status |= 0x02;
    status &= ~0x20;
    wait_write();
    outb(MOUSE_CMD, 0x60);
    wait_write();
    outb(MOUSE_DATA, status);

    mouse_write(0xF3); mouse_write(200);
    mouse_write(0xF3); mouse_write(100);
    mouse_write(0xF3); mouse_write(80);

    /* Get Device ID */
    mouse_write(0xF2);
    wait_read();
    uint8_t dev_id = inb(MOUSE_DATA);
    extern void print(const char *s);
    extern void print_hex8(uint8_t n);
    print("mouse device id = 0x");
    print_hex8(dev_id);
    print("\n");

    mouse_write(0xF4);

    cycle = 0;
    mx = 40; my = 12;
    buttons = 0;
    moved = 0;
}

void mouse_handler(void) {
    uint8_t b = inb(MOUSE_DATA);
    packet[cycle++] = b;
    if (cycle < 4) return;
    cycle = 0;

    uint8_t flags = packet[0];
    if (!(flags & 0x08)) return;

    int dx = (int)packet[1];
    int dy = (int)packet[2];
    if (flags & 0x10) dx |= 0xFFFFFF00;
    if (flags & 0x20) dy |= 0xFFFFFF00;

    if (dx != 0 || dy != 0) {
        mx += dx / 2;
        my -= dy / 2;
        if (mx < 0) mx = 0;
        if (mx >= SCREEN_W) mx = SCREEN_W - 1;
        if (my < 0) my = 0;
        if (my >= SCREEN_H) my = SCREEN_H - 1;
        moved = 1;
    }
    buttons = packet[0] & 0x07;
}

int mouse_get_x(void) { return mx; }
int mouse_get_y(void) { return my; }
int mouse_get_buttons(void) { return buttons; }
int mouse_moved(void) { return moved; }
void mouse_clear_moved(void) { moved = 0; }
