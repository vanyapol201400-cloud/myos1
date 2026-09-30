#include <stdint.h>
#include "prog_bin.h"

extern void print(const char *str);
extern void print_uint(uint32_t n);
extern void enter_user_mode(uint32_t entry, uint32_t user_stack);
extern int  myfs_is_mounted(void);
extern int  myfs_read(const char *name, char *buf, uint32_t max);
extern int  myfs_write(const char *name, const char *data, uint32_t len);

#define USER_BASE   0x400000
#define USER_STACK  0x1600000

static uint8_t prog_buf[65536];

/* Установить встроенные программы в ФС */
int install_builtin_programs(void) {
    if (!myfs_is_mounted()) { print("\nFS not mounted\n"); return -1; }
    print("\nInstalling programs...\n");

    if (myfs_write("hello.bin", (const char *)hello_bin, hello_bin_len) < 0)
        { print("  hello.bin: FAILED\n"); return -1; }
    print("  hello.bin: "); print_uint(hello_bin_len); print(" bytes\n");

    if (myfs_write("calc.bin", (const char *)calc_bin, calc_bin_len) < 0)
        { print("  calc.bin: FAILED\n"); return -1; }
    print("  calc.bin: "); print_uint(calc_bin_len); print(" bytes\n");

    if (myfs_write("file_test.bin", (const char *)file_test_bin, file_test_bin_len) < 0)
        { print("  file_test.bin: FAILED\n"); return -1; }
    print("  file_test.bin: "); print_uint(file_test_bin_len); print(" bytes\n");

    if (myfs_write("shell.bin", (const char *)shell_bin, shell_bin_len) < 0)
        { print("  shell.bin: FAILED\n"); return -1; }
    print("  shell.bin: "); print_uint(shell_bin_len); print(" bytes\n");

    print("Install done.\n");
    return 0;
}

static void load_prog(const uint8_t *src, uint32_t len) {
    uint8_t *dst = (uint8_t *)USER_BASE;
    for (int i = 0; i < 65536; i++) dst[i] = 0;
    for (uint32_t i = 0; i < len; i++) dst[i] = src[i];
}

int run_user_program(const char *name) {
    if (!myfs_is_mounted()) { print("\nFS not mounted\n"); return -1; }
    int n = myfs_read(name, (char *)prog_buf, sizeof(prog_buf));
    if (n <= 0) { print("\nNot found: "); print(name); return -1; }

    print("\nLoading "); print(name);
    print(" ("); print_uint((uint32_t)n); print(" bytes)\n");

    load_prog(prog_buf, n);
    print("Entering user mode...\n");
    enter_user_mode(USER_BASE, USER_STACK);
    return 0;
}

/* exec: заменить текущую программу */
void exec_user_program(const char *name) {
    if (!myfs_is_mounted()) { print("\n[exec: FS not mounted]\n"); return; }
    int n = myfs_read(name, (char *)prog_buf, sizeof(prog_buf));
    if (n <= 0) { print("\n[exec: not found]\n"); return; }
    load_prog(prog_buf, n);
    enter_user_mode(USER_BASE, USER_STACK);
}

/* Запустить shell (user) */
void run_user_shell(void) {
    print("\nStarting user shell...\n");
    load_prog(shell_bin, shell_bin_len);
    enter_user_mode(USER_BASE, USER_STACK);
    print("\n[shell exited]\n");
}
