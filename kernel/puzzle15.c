#include <stdint.h>
extern void vga_put_char_at(int,int,char,uint8_t);
extern int keyboard_has_char(void);
extern char keyboard_get_char(void);
extern uint32_t get_ticks(void);

static int board[16];   /* 0 = пустая клетка, 1-15 = плитки */
static int moves = 0;

static void init_solved(void) {
    for (int i = 0; i < 15; i++) board[i] = i + 1;
    board[15] = 0;
    moves = 0;
}

static void shuffle(void) {
    unsigned int seed = get_ticks() * 1103515245 + 12345;
    for (int i = 0; i < 1000; i++) {
        seed = seed * 1103515245 + 12345;
        int dir = (seed >> 16) & 3;
        int empty = -1;
        for (int j = 0; j < 16; j++) if (!board[j]) { empty = j; break; }
        int r = empty / 4, c = empty % 4;
        int nr = r, nc = c;
        if (dir == 0 && r > 0) nr--;
        else if (dir == 1 && r < 3) nr++;
        else if (dir == 2 && c > 0) nc--;
        else if (dir == 3 && c < 3) nc++;
        else continue;
        int t = board[nr * 4 + nc];
        board[nr * 4 + nc] = 0;
        board[empty] = t;
    }
    moves = 0;
}

static int is_solved(void) {
    for (int i = 0; i < 15; i++) if (board[i] != i + 1) return 0;
    return board[15] == 0;
}

static void draw(int solved) {
    for (int y = 0; y < 25; y++)
        for (int x = 0; x < 80; x++)
            vga_put_char_at(x, y, ' ', 0x07);

    const char *t = "15-PUZZLE";
    for (int i = 0; t[i]; i++) vga_put_char_at(35 + i, 1, t[i], 0x1F);

    const char *h = "Arrows: move   R: shuffle   Esc: exit";
    for (int i = 0; h[i]; i++) vga_put_char_at(20 + i, 23, h[i], 0x07);

    /* Счётчик ходов */
    const char *m = "Moves: ";
    for (int i = 0; m[i]; i++) vga_put_char_at(60 + i, 1, m[i], 0x0E);
    char buf[8]; int n = moves, i = 0;
    if (n == 0) buf[i++] = '0';
    while (n > 0) { buf[i++] = '0' + n % 10; n /= 10; }
    for (int j = 0; j < i; j++) vga_put_char_at(67 + j, 1, buf[i - 1 - j], 0x0E);

    /* Поле 4x4, клетка 6x3 символа, начало (24, 6) */
    int ox = 24, oy = 6;
    int cw = 6, ch = 3;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int x = ox + c * cw;
            int y = oy + r * ch;
            int v = board[r * 4 + c];

            if (v == 0) {
                /* пустая — рамка */
                for (int i = 0; i < cw; i++) {
                    vga_put_char_at(x + i, y,     ' ', 0x07);
                    vga_put_char_at(x + i, y + 1, ' ', 0x07);
                    vga_put_char_at(x + i, y + 2, ' ', 0x07);
                }
                /* пунктирная рамка */
                for (int i = 0; i < cw; i++) {
                    vga_put_char_at(x + i, y,     '-', 0x08);
                    vga_put_char_at(x + i, y + 2, '-', 0x08);
                }
                vga_put_char_at(x, y + 1, '|', 0x08);
                vga_put_char_at(x + cw - 1, y + 1, '|', 0x08);
            } else {
                /* цветная плитка */
                uint8_t attr = (v == r * 4 + c + 1) ? 0x2A : 0x1F;  /* зелёный если на месте */
                if (solved) attr = 0x2A;
                /* верхняя и нижняя линии */
                for (int i = 0; i < cw - 1; i++) {
                    vga_put_char_at(x + i, y,     ' ', attr);
                    vga_put_char_at(x + i, y + 2, ' ', attr);
                    vga_put_char_at(x + i, y + 1, ' ', attr);
                }
                /* число */
                char nb[3]; int ni = 0;
                int tmp = v;
                if (tmp >= 10) nb[ni++] = '0' + (tmp / 10);
                nb[ni++] = '0' + (tmp % 10);
                vga_put_char_at(x + 2, y + 1, nb[0], attr);
                if (ni > 1) vga_put_char_at(x + 3, y + 1, nb[1], attr);
            }
        }
    }

    if (solved) {
        const char *w = "*** SOLVED! ***";
        for (int i = 0; w[i]; i++) vga_put_char_at(30 + i, 20, w[i], 0x2F);
    }
}

static void move_empty(int dr, int dc) {
    int empty = -1;
    for (int j = 0; j < 16; j++) if (!board[j]) { empty = j; break; }
    int r = empty / 4, c = empty % 4;
    int nr = r + dr, nc = c + dc;
    if (nr < 0 || nr > 3 || nc < 0 || nc > 3) return;
    int t = board[nr * 4 + nc];
    board[nr * 4 + nc] = 0;
    board[empty] = t;
    moves++;
}

void gui_run_puzzle15(void) {
    init_solved();
    shuffle();
    draw(0);

    while (1) {
        if (keyboard_has_char()) {
            char ch = keyboard_get_char();
            if (ch == 0x1B) return;
            if (ch == 'r' || ch == 'R') { shuffle(); draw(0); continue; }

            if (ch == 0x15 || ch == 'w' || ch == 'W') move_empty(-1, 0);
            else if (ch == 0x17 || ch == 's' || ch == 'S') move_empty(1, 0);
            else if (ch == 0x14 || ch == 'a' || ch == 'A') move_empty(0, -1);
            else if (ch == 0x16 || ch == 'd' || ch == 'D') move_empty(0, 1);

            int solved = is_solved();
            draw(solved);

            if (solved) {
                while (!keyboard_has_char()) { __asm__ volatile ("hlt"); }
                keyboard_get_char();
                return;
            }
        }
    }
}
