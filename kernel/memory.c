#include <stdint.h>
extern void vga_put_char_at(int,int,char,uint8_t);
extern int keyboard_has_char(void);
extern char keyboard_get_char(void);
extern uint32_t get_ticks(void);

/* Memory (найди пару) — 4x4 = 8 пар */
static int cards[16];      /* 0..7 = значение пары, -1 = открыта, -2 = угадана */
static int revealed[16];   /* 1 = открыта */
static int matched[16];    /* 1 = угадана */
static int first = -1;     /* первая открытая карта */
static int second = -1;    /* вторая открытая */
static int moves = 0;
static int pairs_found = 0;
static int best_moves = 999;

static void shuffle(void) {
    unsigned int seed = get_ticks() * 1103515245 + 12345;
    /* 8 пар: 0,0,1,1,2,2,...,7,7 */
    for (int i = 0; i < 16; i++) cards[i] = i / 2;
    /* Фишер-Йетс */
    for (int i = 15; i > 0; i--) {
        seed = seed * 1103515245 + 12345;
        int j = (seed >> 16) % (i + 1);
        int t = cards[i]; cards[i] = cards[j]; cards[j] = t;
    }
}

static void reset(void) {
    for (int i = 0; i < 16; i++) {
        revealed[i] = 0;
        matched[i] = 0;
    }
    first = second = -1;
    moves = 0;
    pairs_found = 0;
    shuffle();
}

static void draw(int cur, int won) {
    for (int y = 0; y < 25; y++)
        for (int x = 0; x < 80; x++)
            vga_put_char_at(x, y, ' ', 0x07);

    const char *t = "MEMORY - find the pairs";
    /* Рекорд */
    if (best_moves < 999) {
        const char *b = "BEST: ";
        for (int k = 0; b[k]; k++) vga_put_char_at(2, 1, b[k], 0x0E);
        char bbuf[4]; int bn = best_moves, bi = 0;
        if (bn == 0) bbuf[bi++] = '0';
        while (bn > 0) { bbuf[bi++] = '0' + bn % 10; bn /= 10; }
        for (int j = 0; j < bi; j++) vga_put_char_at(8 + j, 1, bbuf[bi - 1 - j], 0x0E);
    }
    for (int i = 0; t[i]; i++) vga_put_char_at(28 + i, 1, t[i], 0x1F);

    const char *h = "Arrows: move   Enter: flip   R: restart   Esc: exit";
    for (int i = 0; h[i]; i++) vga_put_char_at(16 + i, 23, h[i], 0x07);

    char buf[8]; int n = moves, i = 0;
    if (n == 0) buf[i++] = '0';
    while (n > 0) { buf[i++] = '0' + n % 10; n /= 10; }
    const char *m = "Moves: ";
    for (int k = 0; m[k]; k++) vga_put_char_at(60 + k, 1, m[k], 0x0E);
    for (int j = 0; j < i; j++) vga_put_char_at(67 + j, 1, buf[i - 1 - j], 0x0E);

    /* Сетка 4x4, клетка 7x3, начало (22, 5) */
    int ox = 22, oy = 5;
    int cw = 7, ch = 3;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int idx = r * 4 + c;
            int x = ox + c * cw;
            int y = oy + r * ch;

            uint8_t attr = 0x1F;
            char show = ' ';
            if (matched[idx]) {
                attr = 0x2A;
                show = '0' + cards[idx];
            } else if (revealed[idx]) {
                attr = 0x4E;
                show = '0' + cards[idx];
            } else {
                attr = 0x17;
                show = '#';
            }
            if (idx == cur && !won) attr |= 0x80; /* мигание? нет, просто ярче */

            /* Рамка */
            for (int k = 0; k < cw - 1; k++) {
                vga_put_char_at(x + k, y,     ' ', attr);
                vga_put_char_at(x + k, y + 2, ' ', attr);
                vga_put_char_at(x + k, y + 1, ' ', attr);
            }
            vga_put_char_at(x + 3, y + 1, show, attr);
        }
    }

    if (won) {
        const char *w = "*** YOU WIN! ***";
        for (int i = 0; w[i]; i++) vga_put_char_at(30 + i, 19, w[i], 0x2F);
    }
}

void gui_run_memory(void) {
    reset();
    int cur = 0;
    draw(cur, 0);

    while (1) {
        if (keyboard_has_char()) {
            char c = keyboard_get_char();
            if (c == 0x1B) return;
            if (c == 'r' || c == 'R') { reset(); draw(cur, 0); continue; }

            if (c == 0x15 || c == 'w' || c == 'W') { if (cur > 3) cur -= 4; }
            else if (c == 0x17 || c == 's' || c == 'S') { if (cur < 12) cur += 4; }
            else if (c == 0x14 || c == 'a' || c == 'A') { if (cur % 4 > 0) cur--; }
            else if (c == 0x16 || c == 'd' || c == 'D') { if (cur % 4 < 3) cur++; }
            else if (c == '\n' || c == ' ') {
                if (!revealed[cur] && !matched[cur]) {
                    revealed[cur] = 1;
                    if (first == -1) first = cur;
                    else if (second == -1) {
                        second = cur;
                        moves++;
                        /* Проверка пары */
                        if (cards[first] == cards[second]) {
                            matched[first] = matched[second] = 1;
                            pairs_found++;
                            first = second = -1;
                        }
                    }
                }
                draw(cur, 0);
                /* Если две открыты и не пара — закрываем с задержкой */
                if (first != -1 && second != -1) {
                    for (volatile int k = 0; k < 50000000; k++) { }
                    revealed[first] = revealed[second] = 0;
                    first = second = -1;
                    draw(cur, 0);
                }
                if (pairs_found == 8) {
                    if (moves < best_moves) best_moves = moves;
                    draw(cur, 1);
                    while (!keyboard_has_char()) { __asm__ volatile ("hlt"); }
                    keyboard_get_char();
                    return;
                }
                continue;
            }
            draw(cur, 0);
        }
    }
}
