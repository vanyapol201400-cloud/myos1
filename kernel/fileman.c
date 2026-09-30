#include <stdint.h>

extern void vga_put_char_at(int x, int y, char c, uint8_t attr);
extern int  keyboard_has_char(void);
extern char keyboard_get_char(void);
extern int  myfs_is_mounted(void);
extern int  myfs_list(void (*cb)(const char *name, uint32_t size));
extern int  run_user_program(const char *name);

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
static int my_strlen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static int ends_with_bin(const char *s) {
    int n = my_strlen(s);
    if (n < 4) return 0;
    return s[n-4] == '.' && s[n-3] == 'b' && s[n-2] == 'i' && s[n-1] == 'n';
}

static char names[64][32];
static uint32_t sizes[64];
static int n_files = 0;

static void collect(const char *name, uint32_t size) {
    if (n_files < 64) {
        for (int i = 0; name[i] && i < 31; i++) names[n_files][i] = name[i];
        names[n_files][31] = 0;
        sizes[n_files] = size;
        n_files++;
    }
}

static void draw_list(int selected) {
    fill_rect(0, 0, 80, 25, ' ', 0x07);
    fill_rect(0, 0, 80, 1, ' ', 0x70);
    draw_str(2, 0, "=== File Manager ===", 0x70);
    fill_rect(0, 24, 80, 1, ' ', 0x70);
    draw_str(2, 24, "Up/Down select  Enter run  Esc exit", 0x70);

    if (n_files == 0) {
        draw_str(2, 4, "(no files)", 0x07);
        return;
    }

    for (int i = 0; i < n_files && i < 22; i++) {
        int y = 2 + i;
        uint8_t attr_name = (i == selected) ? 0x4F : 0x0F;
        uint8_t attr_meta = (i == selected) ? 0x4F : 0x07;

        draw_str(2, y, names[i], attr_name);
        draw_str(30, y, "...", attr_meta);
        print_uint_at(34, y, sizes[i], attr_meta);
        draw_str(42, y, "bytes", attr_meta);

        if (ends_with_bin(names[i])) {
            draw_str(50, y, "[RUN]", (i == selected) ? 0x4F : 0x0A);
        } else {
            draw_str(50, y, "[--]", attr_meta);
        }
    }
}

void gui_run_fileman(void) {
    if (!myfs_is_mounted()) {
        fill_rect(0, 0, 80, 25, ' ', 0x07);
        draw_str(2, 4, "FS not mounted. Use 'mount' in shell.", 0x0C);
        draw_str(2, 6, "Press any key...", 0x07);
        while (!keyboard_has_char()) { __asm__ volatile ("hlt"); }
        keyboard_get_char();
        return;
    }

    n_files = 0;
    myfs_list(collect);

    int selected = 0;
    draw_list(selected);

    while (1) {
        if (!keyboard_has_char()) { __asm__ volatile ("hlt"); continue; }
        char c = keyboard_get_char();

        if (c == 0x1B) return;

        if (c == 0x11) {
            if (selected > 0) selected--;
            draw_list(selected);
            continue;
        }
        if (c == 0x12) {
            if (selected < n_files - 1) selected++;
            draw_list(selected);
            continue;
        }
        if (c == '\n') {
            if (n_files == 0) continue;
            if (!ends_with_bin(names[selected])) {
                draw_str(2, 24, "Only .bin files can be run!         ", 0x1F);
                continue;
            }

            fill_rect(0, 0, 80, 25, ' ', 0x07);
            draw_str(10, 10, "Running: ", 0x0E);
            draw_str(19, 10, names[selected], 0x0F);
            draw_str(10, 12, "Press any key...", 0x07);

            while (!keyboard_has_char()) { __asm__ volatile ("hlt"); }
            keyboard_get_char();

            run_user_program(names[selected]);

            draw_list(selected);
            continue;
        }
        if (c >= '1' && c <= '9') {
            int n = c - '1';
            if (n < n_files) { selected = n; draw_list(selected); }
        }
    }
}
