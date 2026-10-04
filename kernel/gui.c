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

#define N_MAIN  6
#define N_GAMES 9

static int active   = 0;
static int screen   = 0;
static int selected = 0;

/* ---- утилиты ---- */
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
static void header(void) {
    fill_rect(0, 0, 80, 1, 0x70);
    draw_str(2, 0, "MyOS v0.1", 0x70);
    draw_str(70, 0, "GUI", 0x70);
}
static void footer(const char *s) {
    fill_rect(0, 24, 80, 1, 0x70);
    draw_str(2, 24, s, 0x70);
}

/* ---- главное меню ---- */
static void draw_menu(void) {
    clear_all();
    header();
    fill_rect(15, 4, 50, 17, 0x17);
    draw_str(17, 5, "=== MyOS Menu ===", 0x1F);

    draw_str(19, 7,  "1. File manager", (selected == 0) ? 0x4F : 0x1F);
    draw_str(19, 9,  "2. Games",        (selected == 1) ? 0x4F : 0x1F);
    draw_str(19, 11, "3. Calculator",   (selected == 2) ? 0x4F : 0x1F);
    draw_str(19, 13, "4. About MyOS",   (selected == 3) ? 0x4F : 0x1F);
    draw_str(19, 15, "5. Exit GUI",     (selected == 4) ? 0x4F : 0x1F);
    draw_str(19, 17, "6. Reboot",       (selected == 5) ? 0x4F : 0x1F);

    footer("Up/Down: select  Enter: choose  Esc: exit to shell");
}

/* ---- меню игр (2 столбца) ---- */
static void draw_games(void) {
    clear_all();
    header();
    fill_rect(5, 3, 70, 19, 0x17);
    draw_str(33, 4, "=== Games ===", 0x1F);

    /* левый столбец 0..5 */
    draw_str(10,  6, "1. Snake",        (selected==0)?0x4F:0x17);
    draw_str(10,  8, "2. Pong",         (selected==1)?0x4F:0x17);
    draw_str(10, 10, "3. Tetris",       (selected==2)?0x4F:0x17);
    draw_str(10, 12, "4. 2048",         (selected==3)?0x4F:0x17);
    draw_str(10, 14, "5. Minesweeper",  (selected==4)?0x4F:0x17);
    draw_str(10, 16, "6. Tic-tac-toe",  (selected==5)?0x4F:0x17);

    /* правый столбец 6..8 */
    draw_str(45,  6, "7. Life",         (selected==6)?0x4F:0x17);
    draw_str(45,  8, "8. 15-puzzle",    (selected==7)?0x4F:0x17);
    draw_str(45, 10, "9. Back to menu", (selected==8)?0x4F:0x17);

    footer("Up/Down: select  Left/Right: column  Enter: play  Esc: back");
}

/* ---- окно About ---- */
static void draw_window(void) {
    clear_all();
    header();
    fill_rect(5, 3, 70, 19, 0x17);
    draw_str(7, 4, "About MyOS", 0x1F);
    draw_str(7, 6, "MyOS v0.1", 0x1F);
    draw_str(7, 8, "Simple 32-bit OS", 0x1F);
    draw_str(7, 10, "C + NASM, QEMU", 0x1F);
    draw_str(7, 12, "Games + GUI + Shell", 0x1F);
    footer("Esc: back");
}

/* ---- API ---- */
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
}

int  gui_active(void) { return active; }
void gui_exit(void)   { active = 0; }

void gui_key(int key) {
    if (!active) return;

    /* Окно */
    if (screen == 2) {
        if (key == 0x1B) { screen = 0; selected = 0; gui_draw(); }
        return;
    }

    /* Меню игр */
    if (screen == 1) {
        if (key == 0x11) { if (selected > 0) selected--; gui_draw(); }
        else if (key == 0x12) { if (selected < N_GAMES - 1) selected++; gui_draw(); }
        else if (key == 0x14) { if (selected >= 6) selected -= 6; gui_draw(); }
        else if (key == 0x16) { if (selected < 6 && selected + 6 < N_GAMES) selected += 6; gui_draw(); }
        else if (key == '\n') {
            if      (selected == 0) { gui_run_snake();        gui_draw(); }
            else if (selected == 1) { gui_run_pong();         gui_draw(); }
            else if (selected == 2) { gui_run_tetris();       gui_draw(); }
            else if (selected == 3) { gui_run_2048();         gui_draw(); }
            else if (selected == 4) { gui_run_minesweeper();  gui_draw(); }
            else if (selected == 5) { gui_run_tictactoe();    gui_draw(); }
            else if (selected == 6) { gui_run_life();         gui_draw(); }
            else if (selected == 7) { gui_run_puzzle15();     gui_draw(); }
            else                    { screen = 0; selected = 0; gui_draw(); }
        }
        else if (key == 0x1B) { screen = 0; selected = 0; gui_draw(); }
        else if (key >= '1' && key <= '9') { selected = key - '1'; gui_draw(); }
        return;
    }

    /* Главное меню */
    if (key == 0x11) { if (selected > 0) selected--; gui_draw(); }
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
