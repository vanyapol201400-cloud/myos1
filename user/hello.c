static void sys_print(const char *s) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(s)); }
static void sys_putchar(char c) { __asm__ volatile ("int $0x80" : : "a"(1), "b"(c)); }
static void sys_exit(void) { __asm__ volatile ("int $0x80" : : "a"(0)); }
void _start(void) __attribute__((section(".text._start")));
void _start(void) {
    sys_print("Hello from user program!\n");
    sys_print("Running in ring 3.\n");
    for (int i = 0; i < 5; i++) { sys_putchar('0' + i); sys_putchar(' '); }
    sys_putchar('\n');
    sys_print("Bye!\n");
    sys_exit();
}
