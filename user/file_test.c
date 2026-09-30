static void sys_print(const char *s) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(s)); }
static void sys_exit(void) { __asm__ volatile ("int $0x80" : : "a"(0)); }
static unsigned int my_strlen(const char *s) { unsigned int n = 0; while (s[n]) n++; return n; }
static int sys_write(const char *name, const char *data, unsigned int len) {
    struct req { char name[64]; const char *data; unsigned int len; } r;
    for (int i = 0; i < 64; i++) r.name[i] = 0;
    int i = 0; while (name[i] && i < 63) { r.name[i] = name[i]; i++; }
    r.data = data; r.len = len;
    unsigned int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(5), "b"(&r));
    return (int)ret;
}
void _start(void) __attribute__((section(".text._start")));
void _start(void) {
    sys_print("File test: writing...\n");
    const char *msg = "Written by user!\n";
    sys_write("from_user.txt", msg, my_strlen(msg));
    sys_print("Done. Check with 'cat from_user.txt'\n");
    sys_exit();
}
