#include <stdint.h>

extern void vga_put_char_at(int x, int y, char c, uint8_t attr);
extern int  keyboard_has_char(void);
extern char keyboard_get_char(void);
extern uint32_t get_ticks(void);

static void fill_rect(int x, int y, int w, int h, char c, uint8_t attr) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            vga_put_char_at(x + i, y + j, c, attr);
}
static void draw_str(int x, int y, const char *s, uint8_t attr) {
    for (int i = 0; s[i]; i++) vga_put_char_at(x + i, y, s[i], attr);
}
static void print_num(int x, int y, int n, uint8_t attr) {
    char buf[12]; int i = 0;
    if (n == 0) buf[i++] = '0';
    while (n > 0) { buf[i++] = '0' + n % 10; n /= 10; }
    for (int j = 0; j < i; j++) vga_put_char_at(x + j, y, buf[i - 1 - j], attr);
}

/* ==================== SNAKE ==================== */
#define W 40
#define H 20
#define MAXLEN 200

void gui_run_snake(void) {
    fill_rect(0, 0, 80, 25, ' ', 0x07);

    int snake_x[MAXLEN], snake_y[MAXLEN];
    int len = 3;
    int dx = 1, dy = 0;
    int food_x = 15, food_y = 10;
    int score = 0, over = 0;
    unsigned int seed = 12345;

    snake_x[0] = 10; snake_y[0] = 10;
    snake_x[1] = 9;  snake_y[1] = 10;
    snake_x[2] = 8;  snake_y[2] = 10;

    for (int x = 0; x < W; x++) {
        vga_put_char_at(x + 2, 2, '#', 0x0A);
        vga_put_char_at(x + 2, 2 + H, '#', 0x0A);
    }
    for (int y = 0; y <= H; y++) {
        vga_put_char_at(2, 2 + y, '#', 0x0A);
        vga_put_char_at(2 + W, 2 + y, '#', 0x0A);
    }
    draw_str(45, 4, "SNAKE", 0x1F);
    draw_str(45, 6, "Score:", 0x0F);
    draw_str(45, 10, "w a s d", 0x0E);
    draw_str(45, 14, "Esc - exit", 0x07);

    for (int i = 0; i < len; i++)
        vga_put_char_at(2 + snake_x[i], 2 + snake_y[i], i == 0 ? '@' : 'o', i == 0 ? 0x0E : 0x0A);
    vga_put_char_at(2 + food_x, 2 + food_y, '*', 0x0C);

    unsigned int last_tick = get_ticks();

    while (!over) {
        if (keyboard_has_char()) {
            char c = keyboard_get_char();
            if (c == 0x1B) return;
            if ((c == 'w' || c == 0x15) && dy != 1) { dx = 0; dy = -1; }
            else if ((c == 's' || c == 0x17) && dy != -1) { dx = 0; dy = 1; }
            else if (c == 'a' && dx != 1) { dx = -1; dy = 0; }
            else if (c == 'd' && dx != -1) { dx = 1; dy = 0; }
        }

        unsigned int now = get_ticks();
        if (now - last_tick < 15) continue;
        last_tick = now;

        int nx = snake_x[0] + dx;
        int ny = snake_y[0] + dy;

        if (nx <= 0 || nx >= W - 1 || ny <= 0 || ny >= H - 1) { over = 1; break; }
        for (int i = 0; i < len; i++)
            if (snake_x[i] == nx && snake_y[i] == ny) { over = 1; break; }
        if (over) break;

        int ate = (nx == food_x && ny == food_y);

        if (!ate) {
            int tail = len - 1;
            vga_put_char_at(2 + snake_x[tail], 2 + snake_y[tail], ' ', 0x07);
            for (int i = len - 1; i > 0; i--) {
                snake_x[i] = snake_x[i-1];
                snake_y[i] = snake_y[i-1];
            }
        } else {
            if (len < MAXLEN) {
                snake_x[len] = snake_x[len-1];
                snake_y[len] = snake_y[len-1];
                len++;
            }
            for (int i = len - 1; i > 0; i--) {
                snake_x[i] = snake_x[i-1];
                snake_y[i] = snake_y[i-1];
            }
        }

        snake_x[0] = nx;
        snake_y[0] = ny;

        for (int i = 0; i < len; i++)
            vga_put_char_at(2 + snake_x[i], 2 + snake_y[i], i == 0 ? '@' : 'o', i == 0 ? 0x0E : 0x0A);

        if (ate) {
            score += 10;
            seed = seed * 1103515245 + 12345;
            food_x = 1 + (seed >> 16) % (W - 2);
            food_y = 1 + (seed >> 24) % (H - 2);
            vga_put_char_at(2 + food_x, 2 + food_y, '*', 0x0C);
            print_num(53, 6, score, 0x0F);
        }
    }

    draw_str(10, 12, "GAME OVER", 0x4F);
    draw_str(10, 13, "Score:", 0x0F);
    print_num(17, 13, score, 0x0F);
    draw_str(10, 15, "Press any key...", 0x07);
    while (!keyboard_has_char()) { __asm__ volatile ("hlt"); }
    keyboard_get_char();
}

/* ==================== PONG ==================== */
void gui_run_pong(void) {
    fill_rect(0, 0, 80, 25, ' ', 0x07);

    int ball_x = 40, ball_y = 12;
    int ball_dx = 1, ball_dy = 1;
    int pad_y = 10;
    int score = 0;
    int over = 0;

    for (int x = 2; x < 78; x++) {
        vga_put_char_at(x, 2, '=', 0x0A);
        vga_put_char_at(x, 22, '=', 0x0A);
    }
    draw_str(35, 0, "PONG", 0x1F);
    draw_str(60, 4, "Score:", 0x0F);

    for (int y = 0; y < 4; y++) vga_put_char_at(3, pad_y + y, '|', 0x0E);

    unsigned int last_tick = get_ticks();

    while (!over) {
        if (keyboard_has_char()) {
            char c = keyboard_get_char();
            if (c == 0x1B) return;
            if ((c == 'w' || c == 0x15) && pad_y > 3) {
                vga_put_char_at(3, pad_y + 3, ' ', 0x07);
                pad_y--;
                vga_put_char_at(3, pad_y, '|', 0x0E);
            } else if ((c == 's' || c == 0x17) && pad_y < 18) {
                vga_put_char_at(3, pad_y, ' ', 0x07);
                pad_y++;
                vga_put_char_at(3, pad_y + 3, '|', 0x0E);
            }
        }

        unsigned int now = get_ticks();
        if (now - last_tick < 5) continue;
        last_tick = now;

        vga_put_char_at(ball_x, ball_y, ' ', 0x07);

        ball_x += ball_dx;
        ball_y += ball_dy;

        if (ball_y <= 3 || ball_y >= 21) ball_dy = -ball_dy;
        if (ball_x <= 4) {
            if (ball_y >= pad_y && ball_y <= pad_y + 3) {
                ball_dx = -ball_dx;
                score++;
                print_num(67, 4, score, 0x0F);
            } else {
                over = 1;
                break;
            }
        }
        if (ball_x >= 77) ball_dx = -ball_dx;

        vga_put_char_at(ball_x, ball_y, 'O', 0x0C);
    }

    draw_str(30, 12, "GAME OVER", 0x4F);
    draw_str(28, 14, "Press any key...", 0x07);
    while (!keyboard_has_char()) { __asm__ volatile ("hlt"); }
    keyboard_get_char();
}

/* ==================== TETRIS ==================== */
#define TW 10
#define TH 20
#define OX 30
#define OY 3

/* Фигуры: 7 штук, каждая 4x4, 4 поворота */
static const uint8_t pieces[7][4][16] = {
    /* I */
    {{0,0,0,0, 1,1,1,1, 0,0,0,0, 0,0,0,0}, {0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
     {0,0,0,0, 0,0,0,0, 1,1,1,1, 0,0,0,0}, {0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0}},
    /* O */
    {{0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0}, {0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0},
     {0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0}, {0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0}},
    /* T */
    {{0,1,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0}, {0,1,0,0, 0,1,1,0, 0,1,0,0, 0,0,0,0},
     {0,0,0,0, 1,1,1,0, 0,1,0,0, 0,0,0,0}, {0,1,0,0, 1,1,0,0, 0,1,0,0, 0,0,0,0}},
    /* L */
    {{0,1,0,0, 0,1,0,0, 0,1,1,0, 0,0,0,0}, {0,0,0,0, 1,1,1,0, 1,0,0,0, 0,0,0,0},
     {1,1,0,0, 0,1,0,0, 0,1,0,0, 0,0,0,0}, {0,0,1,0, 1,1,1,0, 0,0,0,0, 0,0,0,0}},
    /* J */
    {{0,1,0,0, 0,1,0,0, 1,1,0,0, 0,0,0,0}, {1,0,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0},
     {0,1,1,0, 0,1,0,0, 0,1,0,0, 0,0,0,0}, {0,0,0,0, 1,1,1,0, 0,0,1,0, 0,0,0,0}},
    /* S */
    {{0,1,1,0, 1,1,0,0, 0,0,0,0, 0,0,0,0}, {0,1,0,0, 0,1,1,0, 0,0,1,0, 0,0,0,0},
     {0,0,0,0, 0,1,1,0, 1,1,0,0, 0,0,0,0}, {1,0,0,0, 1,1,0,0, 0,1,0,0, 0,0,0,0}},
    /* Z */
    {{1,1,0,0, 0,1,1,0, 0,0,0,0, 0,0,0,0}, {0,0,1,0, 0,1,1,0, 0,1,0,0, 0,0,0,0},
     {0,0,0,0, 1,1,0,0, 0,1,1,0, 0,0,0,0}, {0,1,0,0, 1,1,0,0, 1,0,0,0, 0,0,0,0}},
};

static const uint8_t colors[7] = {
    0x0B,  /* I - cyan */
    0x0E,  /* O - yellow */
    0x0D,  /* T - magenta */
    0x0C,  /* L - red */
    0x09,  /* J - blue */
    0x0A,  /* S - green */
    0x0F,  /* Z - white */
};

static int board[TH][TW];
static int cur_x, cur_y, cur_piece, cur_rot;
static int next_piece = 0;
static int score_t = 0;
static int tet_level = 1;
static int tet_lines_total = 0;

static int check_collision(int nx, int ny, int rot) {
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (!pieces[cur_piece][rot][py*4 + px]) continue;
            int bx = nx + px;
            int by = ny + py;
            if (bx < 0 || bx >= TW) return 1;
            if (by >= TH) return 1;
            if (by >= 0 && board[by][bx]) return 1;
        }
    }
    return 0;
}

static void draw_piece_at(int nx, int ny, int rot, int color) {
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (!pieces[cur_piece][rot][py*4 + px]) continue;
            int bx = nx + px;
            int by = ny + py;
            if (bx >= 0 && bx < TW && by >= 0 && by < TH) {
                vga_put_char_at(OX + bx * 2, OY + by, '[', color);
                vga_put_char_at(OX + bx * 2 + 1, OY + by, ']', color);
            }
        }
    }
}

static void erase_piece_at(int nx, int ny, int rot) {
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (!pieces[cur_piece][rot][py*4 + px]) continue;
            int bx = nx + px;
            int by = ny + py;
            if (bx >= 0 && bx < TW && by >= 0 && by < TH) {
                vga_put_char_at(OX + bx * 2, OY + by, ' ', 0x07);
                vga_put_char_at(OX + bx * 2 + 1, OY + by, ' ', 0x07);
            }
        }
    }
}

static void draw_board(void) {
    for (int y = 0; y < TH; y++)
        for (int x = 0; x < TW; x++) {
            if (board[y][x]) {
                vga_put_char_at(OX + x * 2, OY + y, '[', board[y][x]);
                vga_put_char_at(OX + x * 2 + 1, OY + y, ']', board[y][x]);
            } else {
                vga_put_char_at(OX + x * 2, OY + y, ' ', 0x07);
                vga_put_char_at(OX + x * 2 + 1, OY + y, ' ', 0x07);
            }
        }
}

static void lock_piece(void) {
    uint8_t color = colors[cur_piece];
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (!pieces[cur_piece][cur_rot][py*4 + px]) continue;
            int bx = cur_x + px;
            int by = cur_y + py;
            if (by >= 0 && by < TH && bx >= 0 && bx < TW)
                board[by][bx] = color;
        }
    }
    /* Проверка линий */
    int lines = 0;
    for (int y = TH - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < TW; x++) if (!board[y][x]) { full = 0; break; }
        if (full) {
            lines++;
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < TW; x++)
                    board[yy][x] = board[yy-1][x];
            for (int x = 0; x < TW; x++) board[0][x] = 0;
            y++;
        }
    }
    if (lines) {
        score_t += lines * 100;
        print_num(45, 6, score_t, 0x0F);
    }
}

static void draw_next_preview(void) {
    int px = 62, py = 8;
    /* Заголовок */
    draw_str(px, py - 2, "NEXT:", 0x1F);
    /* Очистить область 6x4 */
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 6; x++)
            vga_put_char_at(px + x, py + y, ' ', 0x07);

    /* Нарисовать фигуру next_piece (rot=0) в маленьком виде */
    for (int yy = 0; yy < 4; yy++) {
        for (int xx = 0; xx < 4; xx++) {
            if (pieces[next_piece][0][yy*4 + xx]) {
                vga_put_char_at(px + xx, py + yy, '#', colors[next_piece]);
            }
        }
    }
}

void gui_run_tetris(void) {
    fill_rect(0, 0, 80, 25, ' ', 0x07);

    /* Рамка */
    for (int y = 0; y < TH; y++) {
        vga_put_char_at(OX - 1, OY + y, '|', 0x0A);
        vga_put_char_at(OX + TW * 2, OY + y, '|', 0x0A);
    }
    for (int x = 0; x < TW * 2 + 1; x++) {
        vga_put_char_at(OX - 1 + x, OY - 1, '-', 0x0A);
        vga_put_char_at(OX - 1 + x, OY + TH, '-', 0x0A);
    }

    draw_str(50, 3, "TETRIS", 0x1F);
    draw_str(50, 5, "Score:", 0x0F);
    draw_str(50, 9, "a / d - move", 0x0E);
    draw_str(50, 10, "w - rotate", 0x0E);
    draw_str(50, 11, "s - down", 0x0E);
    draw_str(50, 14, "Esc - exit", 0x07);

    /* Обнулить */
    for (int y = 0; y < TH; y++)
        for (int x = 0; x < TW; x++)
            board[y][x] = 0;
    score_t = 0;
    tet_level = 1;
    tet_lines_total = 0;
    draw_board();

    /* Спавн */
    unsigned int seed = get_ticks();
    int over = 0;
    unsigned int last_tick = get_ticks();

    /* Первая следующая фигура */
    seed = seed * 1103515245 + 12345;
    next_piece = (seed >> 8) % 7;

    while (!over) {
        /* Текущая = та, что была next */
        cur_piece = next_piece;
        cur_rot = 0;
        cur_x = TW / 2 - 2;
        cur_y = 0;

        /* Сгенерировать новую next */
        seed = seed * 1103515245 + 12345;
        next_piece = (seed >> 8) % 7;

        draw_next_preview();

        if (check_collision(cur_x, cur_y, cur_rot)) { over = 1; break; }

        /* Играем фигуру */
        int landed = 0;
        while (!landed) {
            draw_piece_at(cur_x, cur_y, cur_rot, colors[cur_piece]);

            unsigned int now = get_ticks();
            int drop_speed = 50 - (tet_level - 1) * 4;
            if (drop_speed < 10) drop_speed = 10;
            int drop = (now - last_tick > drop_speed);
            if (drop) last_tick = now;

            if (keyboard_has_char()) {
                char c = keyboard_get_char();
                if (c == 0x1B) return;

                if (c == 'a' && !check_collision(cur_x - 1, cur_y, cur_rot)) {
                    erase_piece_at(cur_x, cur_y, cur_rot);
                    cur_x--;
                } else if (c == 'd' && !check_collision(cur_x + 1, cur_y, cur_rot)) {
                    erase_piece_at(cur_x, cur_y, cur_rot);
                    cur_x++;
                } else if (c == 'w') {
                    int nr = (cur_rot + 1) % 4;
                    if (!check_collision(cur_x, cur_y, nr)) {
                        erase_piece_at(cur_x, cur_y, cur_rot);
                        cur_rot = nr;
                    }
                } else if (c == 's') {
                    if (!check_collision(cur_x, cur_y + 1, cur_rot)) {
                        erase_piece_at(cur_x, cur_y, cur_rot);
                        cur_y++;
                    } else {
                        landed = 1;
                    }
                }
            }

            if (drop) {
                if (!check_collision(cur_x, cur_y + 1, cur_rot)) {
                    erase_piece_at(cur_x, cur_y, cur_rot);
                    cur_y++;
                } else {
                    landed = 1;
                }
            }
        }

        lock_piece();
        draw_board();
    }

    draw_str(30, 12, "GAME OVER", 0x4F);
    draw_str(28, 14, "Press any key...", 0x07);
    while (!keyboard_has_char()) { __asm__ volatile ("hlt"); }
    keyboard_get_char();
}
