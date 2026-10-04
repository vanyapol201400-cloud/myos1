#include <stdint.h>
extern void vga_put_char_at(int,int,char,uint8_t);
extern int keyboard_has_char(void);
extern char keyboard_get_char(void);
extern uint32_t get_ticks(void);

#define LW 60
#define LH 22
#define OX 10
#define OY 2

static uint8_t cur[LH][LW];
static uint8_t nxt[LH][LW];

static void life_clear(void) {
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++)
            cur[y][x] = nxt[y][x] = 0;
}

static void life_random(void) {
    unsigned int seed = get_ticks() * 1103515245 + 12345;
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++) {
            seed = seed * 1103515245 + 12345;
            cur[y][x] = (seed >> 16) & 1;
        }
}

static void life_step(void) {
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++) {
            int n = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = (x + dx + LW) % LW;
                    int ny = (y + dy + LH) % LH;
                    n += cur[ny][nx];
                }
            if (cur[y][x]) nxt[y][x] = (n == 2 || n == 3) ? 1 : 0;
            else          nxt[y][x] = (n == 3) ? 1 : 0;
        }
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++)
            cur[y][x] = nxt[y][x];
}

static void life_draw(int gen) {
    for (int y = 0; y < LH; y++)
        for (int x = 0; x < LW; x++) {
            char c = cur[y][x] ? 0xDB : ' ';
            uint8_t a = cur[y][x] ? 0x0A : 0x07;
            vga_put_char_at(OX + x, OY + y, c, a);
        }
    const char *t = "CONWAY'S GAME OF LIFE";
    for (int i = 0; t[i]; i++) vga_put_char_at(30 + i, 0, t[i], 0x1F);
    const char *h = "R:random  C:clear  Space:pause  Esc:exit  Gen:";
    for (int i = 0; h[i]; i++) vga_put_char_at(5 + i, 24, h[i], 0x07);
    char buf[8]; int i = 0, g = gen;
    if (g == 0) buf[i++] = '0';
    while (g > 0) { buf[i++] = '0' + g % 10; g /= 10; }
    for (int j = 0; j < i; j++) vga_put_char_at(56 + j, 24, buf[i - 1 - j], 0x0E);
}

void gui_run_life(void) {
    for (int y = 0; y < 25; y++)
        for (int x = 0; x < 80; x++)
            vga_put_char_at(x, y, ' ', 0x07);

    life_clear();
    life_random();

    int gen = 0;
    int paused = 0;
    unsigned int last = get_ticks();

    life_draw(gen);

    while (1) {
        if (keyboard_has_char()) {
            char c = keyboard_get_char();
            if (c == 27 || c == 0x1B) return;
            if (c == 'r' || c == 'R') { life_random(); gen = 0; life_draw(gen); }
            if (c == 'c' || c == 'C') { life_clear();  gen = 0; life_draw(gen); }
            if (c == ' ') paused = !paused;
        }

        if (paused) continue;

        unsigned int now = get_ticks();
        if (now - last < 10) continue;
        last = now;

        life_step();
        gen++;
        life_draw(gen);
    }
}
