#include "gui.h"
#include <stdint.h>

extern void vga_put_char_at(int x, int y, char c, uint8_t attr);
extern void gui_run_calc(void);
extern void gui_run_fileman(void);
extern void gui_run_snake(void);
extern void gui_run_pong(void);
extern void gui_run_tetris(void);
extern void gui_run_minesweeper(void);
extern void gui_run_2048(void);
extern void gui_run_tictactoe(void);
extern void gui_run_life(void);
extern void gui_run_puzzle15(void);
extern void gui_run_memory(void);
extern void speaker_click(void);

#define N_MAIN  6
#define N_GAMES 10

static int active   = 0;
static int screen   = 0;
static int selected = 0;
static int blink_state = 0;
static int easter_egg = 0;
static int easter_seq = 0;

/* ---------- утилиты ---------- */
static void clear_all(void) {
    for (int y = 0; y < 25; y++)
        for (int x = 0; x < 80; x++)
            vga_put_char_at(x, y, ' ', 0x07);
}

static void draw_str(int x, int y, const char *s, uint8_t attr) {
    for (int i = 0; s[i]; i++)
        vga_put_char_at(x + i, y, s[i], attr);
}

static void fill_rect(int x, int y, int w, int h, uint8_t attr) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            vga_put_char_at(x + i, y + j, ' ', attr);
}

/* Простая рамка (ASCII) */
static void draw_frame(int x, int y, int w, int h, uint8_t attr) {
    vga_put_char_at(x,         y,         '+', attr);
    vga_put_char_at(x + w - 1, y,         '+', attr);
    vga_put_char_at(x,         y + h - 1, '+', attr);
    vga_put_char_at(x + w - 1, y + h - 1, '+', attr);
    for (int i = 1; i < w - 1; i++) {
        vga_put_char_at(x + i, y,         '-', attr);
        vga_put_char_at(x + i, y + h - 1, '-', attr);
    }
    for (int j = 1; j < h - 1; j++) {
        vga_put_char_at(x,         y + j, '|', attr);
        vga_put_char_at(x + w - 1, y + j, '|', attr);
    }
}

extern uint32_t get_ticks(void);

extern void rtc_read_time(uint8_t *h, uint8_t *m, uint8_t *s);

void draw_clock(int x, int y) {
    uint8_t h, m, s;
    rtc_read_time(&h, &m, &s);
    /* Стираем старые 8 символов */
    for (int i = 0; i < 8; i++) vga_put_char_at(x + i, y, ' ', 0x1E);
    char buf[9];
    buf[0] = '0' + h / 10; buf[1] = '0' + h % 10;
    buf[2] = ':';
    buf[3] = '0' + m / 10; buf[4] = '0' + m % 10;
    buf[5] = ':';
    buf[6] = '0' + s / 10; buf[7] = '0' + s % 10;
    buf[8] = 0;
    draw_str(x, y, buf, 0x1E);
}

extern void rtc_read_date(uint8_t *d, uint8_t *mo, uint16_t *y);

void draw_date(int x, int y) {
    uint8_t d, mo;
    uint16_t yr;
    rtc_read_date(&d, &mo, &yr);
    /* Стираем старые 10 символов */
    for (int i = 0; i < 10; i++) vga_put_char_at(x + i, y, ' ', 0x1E);
    char buf[11];
    buf[0] = '0' + d / 10;   buf[1] = '0' + d % 10;
    buf[2] = '.';
    buf[3] = '0' + mo / 10;  buf[4] = '0' + mo % 10;
    buf[5] = '.';
    buf[6] = '0' + (yr / 1000) % 10;
    buf[7] = '0' + (yr / 100) % 10;
    buf[8] = '0' + (yr / 10) % 10;
    buf[9] = '0' + yr % 10;
    buf[10] = 0;
    draw_str(x, y, buf, 0x1E);
}

static void header(const char *title) {
    fill_rect(0, 0, 80, 1, 0x1E);
    draw_str(2, 0, "MyOS v0.1", 0x1E);
    if (title) draw_str(33, 0, title, 0x1E);
    draw_clock(66, 0);
}

static void footer(const char *s) {
    fill_rect(0, 24, 80, 1, 0x1E);
    draw_str(2, 24, s, 0x1E);
    draw_date(66, 24);
}


/* Анимация "шторка" — панель расширяется от центра */
static void gui_animate_panel(int cx, int cy, int max_w, int max_h, uint8_t attr) {
    for (int w = 2; w <= max_w; w += 4) {
        int x = cx - w / 2;
        int y = cy - max_h / 2;
        for (int j = 0; j < max_h; j++)
            for (int i = 0; i < w; i++)
                vga_put_char_at(x + i, y + j, ' ', attr);
        for (volatile int s = 0; s < 200000; s++) { }
    }
    /* Финальная отрисовка полного размера */
    fill_rect(cx - max_w / 2, cy - max_h / 2, max_w, max_h, attr);
}

/* ---------- ПАСХАЛКА ---------- */
static void draw_easter(void) {
    clear_all();
    fill_rect(0, 0, 80, 25, 0x0F);
    header("*** SECRET ***");

    const char *msg[] = {
        "    CONGRATULATIONS!    ",
        "                      ",
        "  You found the secret ",
        "  EASTER EGG of MyOS!  ",
        "                      ",
        "  Made by vanyapol     ",
        "  201400-cloud         ",
        "                      ",
        "  GitHub:              ",
        "  vanyapol201400-cloud ",
        "  /myos1               ",
        "                      ",
        "  Press any key...     "
    };
    for (int i = 0; i < 13; i++) {
        draw_str(28, 6 + i, msg[i], 0x0F);
    }
    footer("Esc: back");
}

/* ---------- главное меню ---------- */
static void draw_menu(void) {
    clear_all();
    header("Main Menu");

    gui_animate_panel(40, 12, 51, 17, 0x17);

    fill_rect(16, 5, 51, 17, 0x08);       /* тень */
    fill_rect(15, 4, 51, 17, 0x17);       /* панель */
    draw_frame(15, 4, 51, 17, 0x1E);

    draw_str(30, 4, " MyOS Menu ", 0x1E);

    const char *items[6] = {
        "File manager", "Games", "Calculator",
        "About MyOS", "Exit GUI", "Reboot"
    };
    const char *keys[6] = {"F", "G", "C", "i", "X", "R"};

    for (int i = 0; i < N_MAIN; i++) {
        uint8_t attr  = (i == selected) ? 0x4E : 0x1F;
        uint8_t kattr = (i == selected) ? 0x4E : 0x0E;

        if (i == selected)
            for (int x = 17; x < 64; x++)
                vga_put_char_at(x, 7 + i * 2, ' ', 0x4E);

        if (i == selected && blink_state) {
                vga_put_char_at(16, 7 + i * 2, '>', 0x4E);
            } else if (i == selected) {
                vga_put_char_at(16, 7 + i * 2, ' ', 0x4E);
            }
            draw_str(18, 7 + i * 2, "[", kattr);
        vga_put_char_at(19, 7 + i * 2, keys[i][0], kattr);
        draw_str(20, 7 + i * 2, "] ", kattr);
        draw_str(23, 7 + i * 2, items[i], attr);
    }

    footer("Up/Down: select   Enter: choose   Esc: exit to shell");
}

/* Скроллбар справа */
static void draw_scrollbar(int y, int h, int total, int cur) {
    for (int i = 0; i < h; i++) {
        uint8_t attr = (i == (cur * h / (total > 0 ? total : 1))) ? 0x4E : 0x18;
        vga_put_char_at(76, y + i, 0xB0, attr);  /* ░ */
    }
}

/* ---------- меню игр (2 столбца) ---------- */
static void draw_games(void) {
    clear_all();
    header("Games");

    gui_animate_panel(40, 12, 69, 17, 0x17);

    fill_rect(6, 4, 69, 17, 0x08);
    fill_rect(5, 3, 69, 17, 0x17);
    draw_frame(5, 3, 69, 17, 0x1E);

    draw_str(33, 3, " Games ", 0x1E);

    const char *litems[6] = {
        "Snake", "Pong", "Tetris", "2048", "Minesweeper", "Tic-tac-toe"
    };
    const char *lkeys[6] = {"1","2","3","4","5","6"};

    const char *ritems[4] = {"Life", "15-puzzle", "Memory", "Back to menu"};
    const char *rkeys[4] = {"7","8","9","0"};

    /* левый столбец 0..5 */
    for (int i = 0; i < 6; i++) {
        uint8_t attr  = (i == selected) ? 0x4E : 0x1F;
        uint8_t kattr = (i == selected) ? 0x4E : 0x0E;

        if (i == selected)
            for (int x = 7; x < 38; x++)
                vga_put_char_at(x, 6 + i * 2, ' ', 0x4E);

        if (i == selected && blink_state) {
                vga_put_char_at(6, 6 + i * 2, '>', 0x4E);
            } else if (i == selected) {
                vga_put_char_at(6, 6 + i * 2, ' ', 0x4E);
            }
            draw_str( 8, 6 + i * 2, "[", kattr);
        vga_put_char_at( 9, 6 + i * 2, lkeys[i][0], kattr);
        draw_str(10, 6 + i * 2, "] ", kattr);
        draw_str(13, 6 + i * 2, litems[i], attr);
    }

    /* правый столбец 6..9 */
    for (int i = 0; i < 4; i++) {
        int idx = i + 6;
        uint8_t attr  = (idx == selected) ? 0x4E : 0x1F;
        uint8_t kattr = (idx == selected) ? 0x4E : 0x0E;

        if (idx == selected)
            for (int x = 43; x < 72; x++)
                vga_put_char_at(x, 6 + i * 2, ' ', 0x4E);

        if (idx == selected && blink_state) {
                vga_put_char_at(42, 6 + i * 2, '>', 0x4E);
            } else if (idx == selected) {
                vga_put_char_at(42, 6 + i * 2, ' ', 0x4E);
            }
            draw_str(44, 6 + i * 2, "[", kattr);
        vga_put_char_at(45, 6 + i * 2, rkeys[i][0], kattr);
        draw_str(46, 6 + i * 2, "] ", kattr);
        draw_str(49, 6 + i * 2, ritems[i], attr);
    }

    footer("Up/Down: select  Left/Right: column  Enter: play  Esc: back");
}

/* ---------- окно About ---------- */
static void draw_window(void) {
    clear_all();
    header("About");

    fill_rect(6, 4, 69, 17, 0x08);
    fill_rect(5, 3, 69, 17, 0x17);
    draw_frame(5, 3, 69, 17, 0x1E);

    draw_str(32, 3, " About MyOS ", 0x1E);
    draw_str( 8,  6, "MyOS v0.1", 0x1F);
    draw_str( 8,  8, "Simple 32-bit OS", 0x1F);
    draw_str( 8, 10, "C + NASM + QEMU", 0x1F);
    draw_str( 8, 12, "GUI, Shell, 9 games", 0x1F);
    draw_str( 8, 14, "myfs, virtio, task, user mode", 0x1F);
    draw_str( 8, 16, "by vanyapol201400-cloud", 0x0E);
    draw_str( 8, 18, "github.com/vanyapol201400-cloud/myos1", 0x0E);

    footer("Esc: back");
}

/* ---------- API ---------- */
void gui_init(void) {
    active = 1;
    screen = 0;
    selected = 0;
    gui_draw();
}

void gui_draw(void) {
    if (!active) return;
    if      (screen == 0) draw_menu();
    else if (screen == 1) draw_games();
    else if (screen == 2) draw_window();
    else if (screen == 3) draw_easter();
}

void gui_tick(void) {
    blink_state = !blink_state;
    /* НЕ перерисовываем весь экран — только мигание курсора */
}

int  gui_active(void) { return active; }
void gui_exit(void)   { active = 0; }

void gui_key(int key) {
    if (!active) return;

    /* Пасхалка — Esc для выхода */
    if (screen == 3) {
        if (key == 0x1B) { screen = 0; selected = 0; easter_seq = 0; gui_draw(); }
        return;
    }

    if (screen == 2) {
        if (key == 0x1B) { screen = 0; selected = 0; gui_draw(); }
        return;
    }

    if (screen == 1) {
        if      (key == 0x11) { if (selected > 0) selected--; gui_draw(); }
        else if (key == 0x12) { if (selected < N_GAMES - 1) selected++; gui_draw(); }
        else if (key == 0x14) { if (selected >= 6) selected -= 6; gui_draw(); }
        else if (key == 0x16) { if (selected < 6 && selected + 6 < N_GAMES) selected += 6; gui_draw(); }
        else if (key == '\n') {
            speaker_click();
            if      (selected == 0) { gui_run_snake();        gui_draw(); }
            else if (selected == 1) { gui_run_pong();         gui_draw(); }
            else if (selected == 2) { gui_run_tetris();       gui_draw(); }
            else if (selected == 3) { gui_run_2048();         gui_draw(); }
            else if (selected == 4) { gui_run_minesweeper();  gui_draw(); }
            else if (selected == 5) { gui_run_tictactoe();    gui_draw(); }
            else if (selected == 6) { gui_run_life();         gui_draw(); }
            else if (selected == 7) { gui_run_puzzle15();     gui_draw(); }
            else if (selected == 8) { gui_run_memory();       gui_draw(); }
            else                    { screen = 0; selected = 0; gui_draw(); }
        }
        else if (key == 0x1B) { screen = 0; selected = 0; gui_draw(); }
        else if (key >= '1' && key <= '9') { selected = key - '1'; gui_draw(); }
        return;
    }

    /* Пасхалка: набери MAGIC */
    if (key == 'M' || key == 'm') { easter_seq = 1; easter_egg++; }
    else if (key == 'A' || key == 'a') { easter_seq = (easter_seq == 1) ? 2 : 0; }
    else if (key == 'G' || key == 'g') { easter_seq = (easter_seq == 2) ? 3 : 0; }
    else if (key == 'I' || key == 'i') { easter_seq = (easter_seq == 3) ? 4 : 0; }
    else if (key == 'C' || key == 'c') { easter_seq = (easter_seq == 4) ? 5 : 0; }
    else { easter_seq = 0; }

    if (easter_seq == 5) {
        easter_seq = 0;
        screen = 3;  /* пасхалка */
        draw_easter();
        return;
    }

    if      (key == 0x11) { if (selected > 0) selected--; gui_draw(); }
    else if (key == 0x12) { if (selected < N_MAIN - 1) selected++; gui_draw(); }
    else if (key == '\n') {
        if      (selected == 0) { gui_run_fileman(); gui_draw(); }
        else if (selected == 1) { screen = 1; selected = 0; gui_draw(); }
        else if (selected == 2) { gui_run_calc(); gui_draw(); }
        else if (selected == 3) { screen = 2; gui_draw(); }
        else if (selected == 4) { active = 0; }
        else if (selected == 5) {
            __asm__ volatile ("cli");
            __asm__ volatile ("lidt (0)");
            __asm__ volatile ("int $0");
        }
    }
    else if (key == 0x1B) { active = 0; }
    else if (key >= '1' && key <= '6') { selected = key - '1'; gui_draw(); }
}
