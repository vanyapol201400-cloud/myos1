#include <stdint.h>

extern void vga_put_char_at(int x, int y, char c, uint8_t attr);
extern int  keyboard_has_char(void);
extern char keyboard_get_char(void);
extern uint32_t get_ticks(void);

#define SIZE 4

static int  board[SIZE][SIZE];
static int  score = 0;
static int  game_over = 0;
static unsigned int seed = 12345;

static unsigned int rnd(void) {
    seed = seed * 1103515245 + 12345;
    return (seed >> 16) & 0x7FFF;
}

static void init_game(void) {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = 0;
    score = 0;
    game_over = 0;
    seed = get_ticks();
    if (seed == 0) seed = 12345;
}

static void spawn_tile(void) {
    /* Найти свободные клетки */
    int free_count = 0;
    int free_x[SIZE*SIZE], free_y[SIZE*SIZE];
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (board[i][j] == 0) {
                free_x[free_count] = i;
                free_y[free_count] = j;
                free_count++;
            }
    if (free_count == 0) return;
    int idx = rnd() % free_count;
    board[free_x[idx]][free_y[idx]] = (rnd() % 10 < 9) ? 2 : 4;
}

/* Сдвиг строки влево. Возвращает 1, если что-то сдвинулось. */
static int slide_left(int row[SIZE]) {
    int moved = 0;
    int tmp[SIZE] = {0,0,0,0};
    int j = 0;
    for (int i = 0; i < SIZE; i++) {
        if (row[i] != 0) {
            tmp[j++] = row[i];
        }
    }
    for (int i = 0; i < SIZE - 1; i++) {
        if (tmp[i] != 0 && tmp[i] == tmp[i+1]) {
            tmp[i] *= 2;
            score += tmp[i];
            tmp[i+1] = 0;
            moved = 1;
        }
    }
    int out[SIZE] = {0,0,0,0};
    j = 0;
    for (int i = 0; i < SIZE; i++) {
        if (tmp[i] != 0) out[j++] = tmp[i];
    }
    for (int i = 0; i < SIZE; i++) {
        if (row[i] != out[i]) moved = 1;
        row[i] = out[i];
    }
    return moved;
}

static void rotate_cw(void) {
    int tmp[SIZE][SIZE];
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            tmp[j][SIZE-1-i] = board[i][j];
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = tmp[i][j];
}

static int move_left(void) {
    int moved = 0;
    for (int i = 0; i < SIZE; i++)
        if (slide_left(board[i])) moved = 1;
    return moved;
}

static int move_right(void) {
    rotate_cw(); rotate_cw();
    int moved = move_left();
    rotate_cw(); rotate_cw();
    return moved;
}

static int move_up(void) {
    rotate_cw(); rotate_cw(); rotate_cw();
    int moved = move_left();
    rotate_cw();
    return moved;
}

static int move_down(void) {
    rotate_cw();
    int moved = move_left();
    rotate_cw(); rotate_cw(); rotate_cw();
    return moved;
}

static int can_move(void) {
    /* Есть свободные? */
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (board[i][j] == 0) return 1;
    /* Есть равные соседи? */
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++) {
            if (j < SIZE-1 && board[i][j] == board[i][j+1]) return 1;
            if (i < SIZE-1 && board[i][j] == board[i+1][j]) return 1;
        }
    return 0;
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

static uint8_t tile_attr(int v) {
    if (v == 0)    return 0x08;
    if (v == 2)    return 0x0F;
    if (v == 4)    return 0x0E;
    if (v == 8)    return 0x0C;
    if (v == 16)   return 0x0D;
    if (v == 32)   return 0x0A;
    if (v == 64)   return 0x0B;
    if (v == 128)  return 0x09;
    if (v == 256)  return 0x0F;
    if (v == 512)  return 0x0E;
    if (v == 1024) return 0x0C;
    if (v == 2048) return 0x4F;
    return 0x0F;
}

static void draw(void) {
    fill_rect(0, 0, 80, 25, ' ', 0x07);

    /* Рамка вокруг поля */
    int fx1 = 3, fy1 = 1, fx2 = 3 + 4*9, fy2 = 1 + 4*3;
    /* Углы */
    vga_put_char_at(fx1, fy1, 0xDA, 0x0E);   /* ┌ */
    vga_put_char_at(fx2, fy1, 0xBF, 0x0E);   /* ┐ */
    vga_put_char_at(fx1, fy2, 0xC0, 0x0E);   /* └ */
    vga_put_char_at(fx2, fy2, 0xD9, 0x0E);   /* ┘ */
    /* Горизонтали */
    for (int x = fx1 + 1; x < fx2; x++) {
        vga_put_char_at(x, fy1, 0xC4, 0x0E);  /* ─ */
        vga_put_char_at(x, fy2, 0xC4, 0x0E);
    }
    /* Вертикали */
    for (int y = fy1 + 1; y < fy2; y++) {
        vga_put_char_at(fx1, y, 0xB3, 0x0E);  /* │ */
        vga_put_char_at(fx2, y, 0xB3, 0x0E);
    }

    /* Поле 4x4, каждая клетка 9 символов шириной, 3 высотой */
    int ox = 5, oy = 3;
    int cw = 9, ch = 3;

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int px = ox + j * cw;
            int py = oy + i * ch;
            int v = board[i][j];
            uint8_t attr = tile_attr(v);
            fill_rect(px, py, cw - 1, ch - 1, ' ', attr);
            if (v != 0) {
                /* Найти длину числа */
                int len = 0;
                int tmp = v;
                while (tmp > 0) { len++; tmp /= 10; }
                int tx = px + (cw - 1 - len) / 2;
                int ty = py + (ch - 1) / 2;
                print_uint_at(tx, ty, v, attr);
            }
        }
    }

    /* Счёт */
    draw_str(50, 3, "2048", 0x1F);
    draw_str(50, 5, "Score:", 0x0F);
    print_uint_at(58, 5, score, 0x0E);
    draw_str(50, 8, "Controls:", 0x0F);
    draw_str(50, 9, "W A S D - move", 0x0E);
    draw_str(50, 10, "Arrows - move", 0x0E);
    draw_str(50, 11, "R - restart", 0x0E);
    draw_str(50, 12, "Esc - exit", 0x0E);

    if (game_over) {
        draw_str(50, 14, "GAME OVER", 0x4F);
    }

    /* Кнопки внизу */
    fill_rect(0, 24, 80, 1, ' ', 0x70);
    draw_str(2, 24, "WASD/Arrows: move   R: restart   Esc: exit", 0x70);
}

void gui_run_2048(void) {
    init_game();
    spawn_tile();
    spawn_tile();
    draw();

    while (1) {
        if (!keyboard_has_char()) { __asm__ volatile ("hlt"); continue; }
        char c = keyboard_get_char();

        if (c == 0x1B) return;
        if (c == 'r' || c == 'R') {
            init_game();
            spawn_tile();
            spawn_tile();
            draw();
            continue;
        }

        if (game_over) continue;

        int moved = 0;
        if (c == 0x11 || c == 'a' || c == 'A') moved = move_left();
        else if (c == 0x12 || c == 'd' || c == 'D') moved = move_right();
        else if (c == 0x13 || c == 'w' || c == 'W') moved = move_up();
        else if (c == 0x14 || c == 's' || c == 'S') moved = move_down();

        if (moved) {
            spawn_tile();
            draw();
            if (!can_move()) game_over = 1;
            draw();
        }
    }
}
