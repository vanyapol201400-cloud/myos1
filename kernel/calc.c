#include <stdint.h>

extern void vga_put_char_at(int x, int y, char c, uint8_t attr);
extern int  keyboard_has_char(void);
extern char keyboard_get_char(void);

static void draw_str(int x, int y, const char *s, uint8_t attr) {
    for (int i = 0; s[i]; i++) vga_put_char_at(x + i, y, s[i], attr);
}

static int my_atoi(const char *s) {
    int n = 0, sign = 1;
    while (*s == ' ') s++;
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
    for (int y = 0; y < 25; y++)
        for (int x = 0; x < 80; x++)
            vga_put_char_at(x, y, ' ', 0x07);

    draw_str(2, 1, "=== Calculator ===", 0x1F);
    draw_str(2, 3, "1234567890 +-*/ (Enter=calc, Esc=exit)", 0x0E);
    draw_str(2, 5, "Examples:", 0x0E);
    draw_str(4, 6, "12+34", 0x1F);
    draw_str(4, 7, "5*6", 0x1F);
    draw_str(4, 8, "100/4", 0x1F);
    draw_str(4, 9, "7-3", 0x1F);

    char line[64]; int len = 0; line[0] = 0;
    draw_str(2, 12, "> ", 0x0F);

    while (1) {
        if (keyboard_has_char()) {
            char c = keyboard_get_char();
            if (c == 0x1B) return;

            if (c == '\n') {
                int a = 0, b = 0, res = 0; char op = 0;
                int op_pos = -1;
                for (int i = 0; line[i]; i++) {
                    if (line[i] == '+' || line[i] == '-' || line[i] == '*' || line[i] == '/') {
                        op_pos = i; op = line[i]; break;
                    }
                }
                if (op_pos > 0) {
                    a = my_atoi(line);
                    b = my_atoi(line + op_pos + 1);
                    if (op == '+') res = a + b;
                    else if (op == '-') res = a - b;
                    else if (op == '*') res = a * b;
                    else if (op == '/') res = (b != 0) ? a / b : 0;
                }
                for (int i = 0; i < 20; i++) vga_put_char_at(4 + i, 14, ' ', 0x07);
                draw_str(2, 14, "= ", 0x1F);
                print_int(4, 14, res, 0x2A);

                for (int i = 0; i < 40; i++) vga_put_char_at(2 + i, 12, ' ', 0x07);
                draw_str(2, 12, "> ", 0x0F);
                len = 0; line[0] = 0;
            }
            else if (c == '\b') {
                if (len > 0) { len--; line[len] = 0; }
                for (int i = 0; i < 40; i++) vga_put_char_at(4 + i, 12, ' ', 0x07);
            }
            else if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '*' || c == '/' || c == '.') {
                if (len < 34) { line[len++] = c; line[len] = 0; }
            }
        }

        for (int i = 0; i < len; i++)
            vga_put_char_at(4 + i, 12, line[i], 0x0F);
        vga_put_char_at(4 + len, 12, '_', 0x0E);
    }
}
