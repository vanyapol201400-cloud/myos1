#include <stdint.h>

extern void vga_put_char_at(int x, int y, char c, uint8_t attr);
extern int  keyboard_has_char(void);
extern char keyboard_get_char(void);
extern uint32_t get_ticks(void);

#define W 10
#define H 10
#define MINES 15

static int  field[W][H];
static int  opened[W][H];
static int  flagged[W][H];
static int  cur_x = 0, cur_y = 0;
static int  game_over = 0;
static int  win = 0;
static unsigned int seed = 12345;

static unsigned int rnd(void) {
    seed = seed * 1103515245 + 12345;
    return (seed >> 16) & 0x7FFF;
}
static int count_neighbors(int x, int y) {
    int n = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            if (field[nx][ny] == -1) n++;
        }
    return n;
}
static void init_game(void) {
    for (int x = 0; x < W; x++)
        for (int y = 0; y < H; y++) {
            field[x][y] = 0;
            opened[x][y] = 0;
            flagged[x][y] = 0;
        }
    seed = get_ticks();
    if (seed == 0) seed = 12345;
    int placed = 0;
    while (placed < MINES) {
        int x = rnd() % W;
        int y = rnd() % H;
        if (field[x][y] == -1) continue;
        if (x == 0 && y == 0) continue;
        field[x][y] = -1;
        placed++;
    }
    for (int x = 0; x < W; x++)
        for (int y = 0; y < H; y++)
            if (field[x][y] != -1)
                field[x][y] = count_neighbors(x, y);
    cur_x = 0; cur_y = 0;
    game_over = 0;
    win = 0;
}
static void fill_rect(int x, int y, int w, int h, char c, uint8_t attr) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            vga_put_char_at(x + i, y + j, c, attr);
}
static void draw_str(int x, int y, const char *s, uint8_t attr) {
    for (int i = 0; s[i]; i++) vga_put_char_at(x + i, y, s[i], attr);
}
static void print_uint_at(int x, int y, int n, uint8_t attr) {
    char buf[12]; int i = 0;
    if (n == 0) buf[i++] = '0';
    while (n > 0) { buf[i++] = '0' + n % 10; n /= 10; }
    for (int j = 0; j < i; j++) vga_put_char_at(x + j, y, buf[i - 1 - j], attr);
}
static void draw(void) {
    fill_rect(0, 0, 80, 25, ' ', 0x07);

    /* Рамка вокруг поля */
    int fx1 = 3, fy1 = 1, fx2 = 3 + W*3 + 1, fy2 = 1 + H + 1;
    vga_put_char_at(fx1, fy1, 0xDA, 0x0A);
    vga_put_char_at(fx2, fy1, 0xBF, 0x0A);
    vga_put_char_at(fx1, fy2, 0xC0, 0x0A);
    vga_put_char_at(fx2, fy2, 0xD9, 0x0A);
    for (int x = fx1 + 1; x < fx2; x++) {
        vga_put_char_at(x, fy1, 0xC4, 0x0A);
        vga_put_char_at(x, fy2, 0xC4, 0x0A);
    }
    for (int y = fy1 + 1; y < fy2; y++) {
        vga_put_char_at(fx1, y, 0xB3, 0x0A);
        vga_put_char_at(fx2, y, 0xB3, 0x0A);
    }

    /* Поле 10x10 */
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int px = 4 + x * 3;
            int py = 2 + y;
            uint8_t attr = 0x07;
            char c = 0xDB;   /* закрытая клетка */
            if (opened[x][y]) {
                if (field[x][y] == -1) { c = '*'; attr = 0x4F; }
                else if (field[x][y] == 0) { c = ' '; attr = 0x0F; }
                else { c = '0' + field[x][y]; attr = 0x0F; }
            } else if (flagged[x][y]) {
                c = 'F'; attr = 0x4F;
            }
            if (x == cur_x && y == cur_y && !game_over) attr = 0x4F;
            vga_put_char_at(px, py, c, attr);
            vga_put_char_at(px + 1, py, ' ', 0x07);
        }
    }

    /* Заголовок */
    draw_str(50, 3, "MINESWEEPER", 0x1F);
    draw_str(50, 5, "Mines:", 0x0F);
    print_uint_at(57, 5, MINES, 0x0E);
    draw_str(50, 8, "Controls:", 0x0F);
    draw_str(50, 9, "WASD/Arrows", 0x0E);
    draw_str(50, 10, "Space - open", 0x0E);
    draw_str(50, 11, "F - flag", 0x0E);
    draw_str(50, 12, "R - restart", 0x0E);
    draw_str(50, 13, "Esc - exit", 0x0E);

    if (game_over) {
        draw_str(50, 16, win ? "YOU WIN!" : "GAME OVER", win ? 0x1F : 0x4F);
    }

    /* Кнопки внизу */
    fill_rect(0, 24, 80, 1, ' ', 0x70);
    draw_str(2, 24, "WASD/Arrows: move   Space: open   F: flag   R: restart   Esc: exit", 0x70);
}
static void open_cell(int x, int y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    if (opened[x][y] || flagged[x][y]) return;
    opened[x][y] = 1;
    if (field[x][y] == -1) { game_over = 1; return; }
    if (field[x][y] == 0) {
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                open_cell(x + dx, y + dy);
            }
    }
}
static int check_win(void) {
    for (int x = 0; x < W; x++)
        for (int y = 0; y < H; y++)
            if (field[x][y] != -1 && !opened[x][y]) return 0;
    return 1;
}
void gui_run_minesweeper(void) {
    init_game();
    draw();
    while (1) {
        if (!keyboard_has_char()) { __asm__ volatile ("hlt"); continue; }
        char c = keyboard_get_char();
        /* Отладка: показать код клавиши */
        {
            extern void vga_put_char_at(int x, int y, char c, uint8_t attr);
            const char *hex = "0123456789ABCDEF";
            uint8_t uc = (uint8_t)c;
            vga_put_char_at(75, 0, hex[(uc >> 4) & 0xF], 0x4F);
            vga_put_char_at(76, 0, hex[uc & 0xF], 0x4F);
        }
        if (c == 0x1B) return;
        if (c == 'r' || c == 'R') { init_game(); draw(); continue; }
        if (game_over) continue;
        if (c == 0x11 || c == 'a' || c == 'A') { if (cur_x > 0) cur_x--; draw(); }
        else if (c == 0x12 || c == 'd' || c == 'D') { if (cur_x < W - 1) cur_x++; draw(); }
        else if (c == 0x13 || c == 'w' || c == 'W') { if (cur_y > 0) cur_y--; draw(); }
        else if (c == 0x14 || c == 's' || c == 'S') { if (cur_y < H - 1) cur_y++; draw(); }
        else if (c == ' ') {
            open_cell(cur_x, cur_y);
            if (check_win()) { win = 1; game_over = 1; }
            draw();
        } else if (c == 'f' || c == 'F') {
            if (!opened[cur_x][cur_y]) flagged[cur_x][cur_y] = !flagged[cur_x][cur_y];
            draw();
        }
    }
}
