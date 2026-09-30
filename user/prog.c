/* user-программа: работает с ФС */

static void sys_print(const char *s) {
    __asm__ volatile ("int $0x80" : : "a"(2), "b"(s));
}
static void sys_putchar(char c) {
    __asm__ volatile ("int $0x80" : : "a"(1), "b"(c));
}
static void sys_exit(void) {
    __asm__ volatile ("int $0x80" : : "a"(0));
}

/* read: name + buf + max */
static int sys_read(const char *name, char *buf, unsigned int max) {
    struct req {
        char name[64];
        char *buf;
        unsigned int max;
    } r;
    for (int i = 0; i < 64; i++) r.name[i] = 0;
    int i = 0;
    while (name[i] && i < 63) { r.name[i] = name[i]; i++; }
    r.buf = buf;
    r.max = max;
    unsigned int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(4), "b"(&r));
    return (int)ret;
}

/* write: name + data + len */
static int sys_write(const char *name, const char *data, unsigned int len) {
    struct req {
        char name[64];
        const char *data;
        unsigned int len;
    } r;
    for (int i = 0; i < 64; i++) r.name[i] = 0;
    int i = 0;
    while (name[i] && i < 63) { r.name[i] = name[i]; i++; }
    r.data = data;
    r.len = len;
    unsigned int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(5), "b"(&r));
    return (int)ret;
}

static unsigned int my_strlen(const char *s) {
    unsigned int n = 0;
    while (s[n]) n++;
    return n;
}

void _start(void) {
    sys_print("File test program\n");

    /* Запишем в файл */
    const char *msg = "Hello from user-space file!\n";
    int w = sys_write("user.txt", msg, my_strlen(msg));
    if (w < 0) {
        sys_print("Write failed\n");
    } else {
        sys_print("Wrote file\n");
    }

    /* Прочитаем обратно */
    char buf[256];
    for (int i = 0; i < 256; i++) buf[i] = 0;
    int n = sys_read("user.txt", buf, 255);
    if (n < 0) {
        sys_print("Read failed\n");
    } else {
        sys_print("Read: ");
        sys_print(buf);
    }

    sys_print("Done!\n");
    sys_exit();
}
