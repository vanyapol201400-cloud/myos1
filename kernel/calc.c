#include <stdint.h>

extern void vga_put_char_at(int x, int y, char c, uint8_t attr);
extern int  keyboard_has_char(void);
extern char keyboard_get_char(void);

static void fill_rect(int x, int y, int w, int h, char c, uint8_t attr) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            vga_put_char_at(x + i, y + j, c, attr);
}
static void draw_str(int x, int y, const char *s, uint8_t attr) {
    for (int i = 0; s[i]; i++) vga_put_char_at(x + i, y, s[i], attr);
}
static int my_atoi(const char *s) {
    int n = 0, sign = 1;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n * sign;
}
static void print_int(int x, int y, int n, uint8_t attr) {
    char buf[16]; int i = 0;
    if (n == 0) { vga_put_char_at(x, y, '0', attr); return; }
    if (n < 0) { vga_put_char_at(x, y, '-', attr); x++; n = -n; }
    while (n) { buf[i++] = '0' + n % 10; n /= 10; }
    for (int j = 0; j < i; j++) vga_put_char_at(x + j, y, buf[i - 1 - j], attr);
}

void gui_run_calc(void) {
    fill_rect(0, 0, 80, 25, ' ', 0x07);

    int ox = 20, oy = 4;
    fill_rect(ox, oy, 40, 18, ' ', 0x70);
    draw_str(ox + 2, oy, "=== Calculator ===", 0x70);
    fill_rect(ox + 2, oy + 2, 36, 1, ' ', 0x0F);
    draw_str(ox + 2, oy + 2, ">", 0x0E);

    draw_str(ox + 2, oy + 5, "Examples:", 0x70);
    draw_str(ox + 4, oy + 6, "12 + 34", 0x70);
    draw_str(ox + 4, oy + 7, "5 * 6", 0x70);
    draw_str(ox + 4, oy + 8, "100 / 4", 0x70);
    draw_str(ox + 4, oy + 9, "7 - 3", 0x70);
    draw_str(ox + 2, oy + 12, "Enter - calc, Esc - exit", 0x70);

    char line[64]; int len = 0;
    int result = 0;
    int has_result = 0;

    while (1) {
        for (int i = 0; i < 34; i++) vga_put_char_at(ox + 3 + i, oy + 2, ' ', 0x0F);
        for (int i = 0; i < len; i++) vga_put_char_at(ox + 3 + i, oy + 2, line[i], 0x0F);
        vga_put_char_at(ox + 3 + len, oy + 2, '_', 0x0E);

        if (has_result) {
            draw_str(ox + 2, oy + 14, "= ", 0x1F);
            print_int(ox + 4, oy + 14, result, 0x1F);
        }

        if (!keyboard_has_char()) { __asm__ volatile ("hlt"); continue; }
        char c = keyboard_get_char();

        if (c == 0x1B) return;

        if (c == '\n') {
            int i = 0;
            while (line[i] == ' ') i++;
            char num1s[16]; int n1i = 0;
            while (line[i] >= '0' && line[i] <= '9') num1s[n1i++] = line[i++];
            num1s[n1i] = 0;
            while (line[i] == ' ') i++;
            char op = line[i++];
            while (line[i] == ' ') i++;
            char num2s[16]; int n2i = 0;
            while (line[i] >= '0' && line[i] <= '9') num2s[n2i++] = line[i++];
            num2s[n2i] = 0;

            int a = my_atoi(num1s);
            int b = my_atoi(num2s);
            if (op == '+') result = a + b;
            else if (op == '-') result = a - b;
            else if (op == '*') result = a * b;
            else if (op == '/') result = (b != 0) ? a / b : 0;
            has_result = 1;
            len = 0;
            continue;
        }
        if (c == '\b') { if (len > 0) len--; continue; }
        if (c >= 32 && c < 127 && len < 63) line[len++] = c;
    }
}
