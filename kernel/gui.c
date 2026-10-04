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
extern void gui_run_sudoku(void);

#define N_MAIN 6
#define N_GAMES 9

static int active = 0;
static int screen = 0;       /* 0=menu, 1=games, 2=window, 3=game */
static int selected = 0;
static int window_type = 0;

static char *main_items[N_MAIN] = {
    "1. File manager",
    "2. Games",
    "3. Calculator",
    "4. About MyOS",
    "5. Exit GUI",
    "6. Reboot",
};

static char *game_items[N_GAMES] = {
    "1. Snake",
    "2. Pong",
    "3. Tetris",
    "4. 2048",
    "5. Minesweeper",
    "6. Tic-tac-toe",
    "7. Life",
    "8. 15-puzzle",
    "9. Back to menu",
};

static void draw_str(int x, int y, const char *s, uint8_t attr) {
    for (int i = 0; s[i]; i++) vga_put_char_at(x + i, y, s[i], attr);
}
static void fill_rect(int x, int y, int w, int h, uint8_t attr) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            vga_put_char_at(x + i, y + j, ' ', attr);
}
static void clear_all(void) {
    fill_rect(0, 0, 80, 25, 0x07);
}
static void header(void) {
    fill_rect(0, 0, 80, 1, 0x70);
    fill_rect(0, 24, 80, 1, 0x70);
}
static void footer(const char *s) {
    draw_str(2, 24, s, 0x70);
}

void gui_init(void) {
    active = 1;
    screen = 0;
    selected = 0;
    gui_draw();
}
int gui_active(void) { return active; }
void gui_exit(void) { active = 0; }

static const char *menu_icons[N_MAIN] = {
    "F", "G", "C", "i", "X", "R",
};


static void draw_menu(void) {
    clear_all();
    header();
    fill_rect(15, 4, 50, 17, 0x17);
    draw_str(17, 5, "=== MyOS Menu ===", 0x1F);

    draw_str(19, 7, "TEST_A", 0x4F);
    draw_str(19, 9, "TEST_B", 0x4F);
    draw_str(19, 11, main_items[0], 0x4F);
    draw_str(19, 13, main_items[1], 0x4F);

    for (int i = 0; i < N_MAIN; i++) {
        uint8_t attr = (i == selected) ? 0x4F : 0x1F;
        draw_str(19, 15 + i * 2, main_items[i], attr);
    }
    footer("Up/Down: select  Enter: choose  Esc: exit to shell");
}

static void draw_games(void) {
    clear_all();
    header();
    fill_rect(5, 3, 70, 19, 0x17);
    draw_str(33, 4, "=== Games ===", 0x1F);

    /* Левый столбец: 0..5 (Snake..TTT) */
    for (int i = 0; i < 6 && i < N_GAMES; i++) {
        uint8_t attr = (i == selected) ? 0x4F : 0x17;
        draw_str(10, 6 + i * 2, game_items[i], attr);
    }

    /* Правый столбец: 6..8 (Life, 15-puzzle, Back) */
    for (int i = 6; i < N_GAMES && i < 9; i++) {
        uint8_t attr = (i == selected) ? 0x4F : 0x17;
        draw_str(45, 6 + (i - 6) * 2, game_items[i], attr);
    }

    footer("Up/Down: select  Left/Right: column  Enter: play  Esc: back");
}

static void draw_window(void) {
    clear_all();
    header();
    int wx = 5, wy = 3, ww = 70, wh = 19;
    fill_rect(wx, wy, ww, wh, 0x17);
    draw_str(wx + 2, wy, " MyOS Window ", 0x1F);
    draw_str(wx + ww - 5, wy, "[Esc]", 0x4F);

    if (window_type == 0) {
        draw_str(wx + 2, wy + 2, "File manager:", 0x1F);
        draw_str(wx + 4, wy + 4, "hello.bin       229 bytes", 0x17);
        draw_str(wx + 4, wy + 5, "calc.bin        358 bytes", 0x17);
        draw_str(wx + 4, wy + 6, "file_test.bin   370 bytes", 0x17);
        draw_str(wx + 4, wy + 7, "shell.bin       XXXX bytes", 0x17);
    } else if (window_type == 2) {
        draw_str(wx + 2, wy + 2, "MyOS v0.3", 0x1F);
        draw_str(wx + 4, wy + 4, "User-mode OS from scratch", 0x17);
        draw_str(wx + 4, wy + 5, "GDT + TSS + IDT + paging", 0x17);
        draw_str(wx + 4, wy + 6, "virtio-blk + myfs", 0x17);
        draw_str(wx + 4, wy + 7, "user mode (ring 3) + syscalls", 0x17);
        draw_str(wx + 4, wy + 8, "user shell + GUI + games", 0x17);
        draw_str(wx + 4, wy + 11, "Written by lolcatwer", 0x1F);
    }
    footer("Press Esc to return");
}

void gui_draw(void) {
    if (!active) return;
    if (screen == 0) draw_menu();
    else if (screen == 1) draw_games();
    else if (screen == 2) draw_window();
}

void gui_handle_click(int mx, int my) {
    if (!active) return;
    if (screen == 0) {
        if (my >= 7 && my <= 13 && mx >= 14 && mx <= 50) {
            selected = my - 7;
            if (selected < N_MAIN) { gui_draw(); gui_key('\n'); }
            return;
        }
    }
    if (screen == 1) {
        if (my >= 9 && my <= 13 && mx >= 19 && mx <= 45) {
            selected = my - 9;
            if (selected < N_GAMES) { gui_draw(); gui_key('\n'); }
            return;
        }
    }
    if (screen == 2) {
        screen = 0; selected = 0; gui_draw();
        return;
    }
}

void gui_key(int key) {
    if (!active) return;

    if (screen == 2) {   /* окно */
        if (key == 0x1B) { screen = 0; selected = 0; gui_draw(); }
        return;
    }

    if (screen == 1) {   /* меню игр */
        if (key == 0x11) { if (selected > 0) selected--; gui_draw(); }
        else if (key == 0x12) { if (selected < N_GAMES - 1) selected++; gui_draw(); }
        else if (key == '\n') {
            if (selected == 0) { gui_run_snake(); gui_draw(); }
            else if (selected == 1) { gui_run_pong(); gui_draw(); }
            else if (selected == 2) { gui_run_tetris(); gui_draw(); }
            else if (selected == 3) { gui_run_2048(); gui_draw(); }
            else if (selected == 4) { gui_run_minesweeper(); gui_draw(); }
            else if (selected == 5) { gui_run_tictactoe(); gui_draw(); }
            else { screen = 0; selected = 0; gui_draw(); }
        } else if (key == 0x1B) { screen = 0; selected = 0; gui_draw(); }
        else if (key >= '1' && key <= '8') { selected = key - '1'; gui_draw(); }
        return;
    }

    /* Главное меню */
    if (key == 0x11) { if (selected > 0) selected--; gui_draw(); }
    else if (key == 0x12) { if (selected < N_MAIN - 1) selected++; gui_draw(); }
    else if (key == '\n') {
        if (selected == 0) { gui_run_fileman(); gui_draw(); }
        else if (selected == 1) { screen = 1; selected = 0; gui_draw(); }
        else if (selected == 2) { gui_run_calc(); gui_draw(); }
        else if (selected == 3) { screen = 2; window_type = 2; gui_draw(); }
        else if (selected == 4) { active = 0; }
        else if (selected == 5) {
            __asm__ volatile ("cli");
            __asm__ volatile ("lidt (0)");
            __asm__ volatile ("int $0");
        }
    } else if (key == 0x1B) { active = 0; }
    else if (key >= '1' && key <= '8') { selected = key - '1'; gui_draw(); }
}
