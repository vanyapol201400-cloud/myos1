static void sys_print(const char *s) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(s)); }
static void sys_putchar(char c) { __asm__ volatile ("int $0x80" : : "a"(1), "b"(c)); }
static void sys_exit(void) { __asm__ volatile ("int $0x80" : : "a"(0)); }
static void print_num(int n) {
    if (n == 0) { sys_putchar('0'); return; }
    if (n < 0) { sys_putchar('-'); n = -n; }
    char buf[12]; int i = 0;
    while (n) { buf[i++] = '0' + n % 10; n /= 10; }
    while (i--) sys_putchar(buf[i]);
}
void _start(void) __attribute__((section(".text._start")));
void _start(void) {
    sys_print("Calc: 2 + 3 = "); print_num(2 + 3); sys_putchar('\n');
    sys_print("Calc: 10 * 7 = "); print_num(10 * 7); sys_putchar('\n');
    sys_exit();
}
