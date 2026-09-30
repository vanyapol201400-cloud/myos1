static void sys_print(const char *s) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(s)); }
static void sys_putchar(char c) { __asm__ volatile ("int $0x80" : : "a"(1), "b"(c)); }
static void sys_exit(void) { __asm__ volatile ("int $0x80" : : "a"(0)); }
static int sys_read_key(void) { unsigned int ret; __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(7)); return (int)ret; }
static void sys_cls(void) { __asm__ volatile ("int $0x80" : : "a"(8)); }
static void sys_putchar_at(int x, int y, char c) {
    unsigned int arg = ((y & 0xFF) << 24) | ((x & 0xFF) << 16) | (c & 0xFFFF);
    __asm__ volatile ("int $0x80" : : "a"(11), "b"(arg));
}

static int streq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}
static void print_at(int x, int y, const char *s) {
    while (*s) sys_putchar_at(x++, y, *s++);
}

static char line[128];
static int line_len = 0;

static void cmd_help(void) {
    sys_cls();
    print_at(5, 2, "=== MyOS Shell ===");
    print_at(5, 4, "Commands:");
    print_at(7, 6, "help  - this text");
    print_at(7, 7, "clear - clear screen");
    print_at(7, 8, "box   - draw window");
    print_at(7, 9, "exit  - quit");
    print_at(5, 20, "Press any key to return...");
    while (sys_read_key() == 0) { }
}

static void cmd_box(void) {
    sys_cls();
    int x1 = 10, y1 = 3, x2 = 65, y2 = 17;

    /* Рамка */
    sys_putchar_at(x1, y1, 0xDA);   /* ┌ */
    sys_putchar_at(x2, y1, 0xBF);   /* ┐ */
    sys_putchar_at(x1, y2, 0xC0);   /* └ */
    sys_putchar_at(x2, y2, 0xD9);   /* ┘ */
    for (int x = x1 + 1; x < x2; x++) {
        sys_putchar_at(x, y1, 0xC4);
        sys_putchar_at(x, y2, 0xC4);
    }
    for (int y = y1 + 1; y < y2; y++) {
        sys_putchar_at(x1, y, 0xB3);
        sys_putchar_at(x2, y, 0xB3);
    }

    /* Заголовок */
    print_at(x1 + 2, y1, " MyOS v0.1 ");

    /* Текст внутри */
    print_at(x1 + 3, y1 + 3, "This is a text-mode window.");
    print_at(x1 + 3, y1 + 4, "Drawn with syscall 11 (putchar_at).");
    print_at(x1 + 3, y1 + 6, "VGA 80x25, no graphics mode.");
    print_at(x1 + 3, y1 + 7, "Just characters + colors.");

    /* Кнопка */
    print_at(x1 + 3, y2 - 2, "[ OK ]");
    print_at(x1 + 12, y2 - 2, "[ Cancel ]");

    /* Ждём клавишу */
    print_at(x1 + 3, y2 - 1, "Press any key...");
    while (sys_read_key() == 0) { }
    sys_cls();
}

static void process(void) {
    if (line_len == 0) return;
    line[line_len] = 0;
    if (streq(line, "help")) cmd_help();
    else if (streq(line, "clear")) sys_cls();
    else if (streq(line, "box")) cmd_box();
    else if (streq(line, "exit")) sys_exit();
    else {
        print_at(0, 0, "Unknown: ");
        print_at(9, 0, line);
        print_at(0, 1, "Press any key...");
        while (sys_read_key() == 0) { }
        sys_cls();
    }
}

void _start(void) __attribute__((section(".text._start")));
void _start(void) {
    sys_cls();
    print_at(0, 0, "myos> ");
    while (1) {
        int k = sys_read_key();
        if (k == 0) continue;
        if (k == '\n') {
            process();
            line_len = 0;
            print_at(0, 0, "myos> ");
        } else if (k == '\b') {
            if (line_len > 0) {
                line_len--;
                /* стираем символ на экране */
                for (int i = 0; i < line_len + 6; i++) sys_putchar_at(i, 0, ' ');
                print_at(0, 0, "myos> ");
                for (int i = 0; i < line_len; i++) sys_putchar_at(6 + i, 0, line[i]);
            }
        } else if (k >= 32 && k < 127) {
            if (line_len < 127) {
                line[line_len++] = (char)k;
                sys_putchar_at(6 + line_len - 1, 0, (char)k);
            }
        }
    }
}
