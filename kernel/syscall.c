#include <stdint.h>

extern void putchar(char c);
extern void print(const char *str);
extern void print_uint(uint32_t n);
extern void shell_restart(void);
extern int  myfs_is_mounted(void);
extern int  myfs_read(const char *name, char *buf, uint32_t max);
extern int  myfs_write(const char *name, const char *data, uint32_t len);
extern int  keyboard_has_char(void);
extern char keyboard_get_char(void);
extern void clear_screen(void);
extern void vga_set_cursor(int pos);
extern uint32_t get_ticks(void);

/* Эти функции ДОЛЖНЫ быть до syscall_dispatch */
static void safe_print_user(const char *user_ptr, uint32_t max_len) {
    for (uint32_t i = 0; i < max_len; i++) {
        char c = user_ptr[i];
        if (c == 0) break;
        putchar(c);
    }
}

static void safe_copy_from_user(char *dst, const char *user_src, uint32_t max) {
    for (uint32_t i = 0; i < max; i++) {
        dst[i] = user_src[i];
        if (user_src[i] == 0) return;
    }
    dst[max - 1] = 0;
}

void syscall_dispatch(uint32_t num, uint32_t arg, uint32_t *ret) {
    *ret = 0;
    switch (num) {
        case 0:  /* exit */
            print("\n[user program exited]\n");
            __asm__ volatile (
                "mov %0, %%esp\n"
                "jmp shell_restart\n"
                :
                : "r"(0x90000)
                : "memory"
            );
            while (1) __asm__ volatile ("hlt");
            break;

        case 1:  /* putchar */
            putchar((char)arg);
            break;

        case 2:  /* print string */
            safe_print_user((const char *)arg, 256);
            break;

        case 3:  /* open (проверка) */
            if (!myfs_is_mounted()) { *ret = (uint32_t)-1; break; }
            {
                char name[64];
                safe_copy_from_user(name, (const char *)arg, 64);
                char tmp[2];
                int r = myfs_read(name, tmp, 1);
                *ret = (r >= 0) ? 0 : (uint32_t)-1;
            }
            break;

        case 4:  /* read file */
            if (!myfs_is_mounted()) { *ret = (uint32_t)-1; break; }
            {
                char name[64];
                safe_copy_from_user(name, (const char *)arg, 64);
                uint32_t buf_ptr = *(uint32_t *)(arg + 64);
                uint32_t max_len = *(uint32_t *)(arg + 68);
                static char tmp[4096];
                int n = myfs_read(name, tmp, sizeof(tmp) < max_len ? sizeof(tmp) : max_len);
                if (n < 0) { *ret = (uint32_t)-1; break; }
                char *user_buf = (char *)buf_ptr;
                for (int i = 0; i < n; i++) user_buf[i] = tmp[i];
                *ret = (uint32_t)n;
            }
            break;

        case 5:  /* write file */
            if (!myfs_is_mounted()) {
                print("\n[s5: not mounted]\n");
                *ret = (uint32_t)-1;
                break;
            }
            {
                char name[64];
                safe_copy_from_user(name, (const char *)arg, 64);
                print("\n[s5: name='"); print(name); print("']\n");

                /* Читаем data_ptr и len через safe_copy — по байту */
                uint32_t data_ptr = 0;
                uint32_t len = 0;
                safe_copy_from_user((char *)&data_ptr, (const char *)(arg + 64), 4);
                safe_copy_from_user((char *)&len, (const char *)(arg + 68), 4);

                print("[s5: data_ptr=0x"); print_uint(data_ptr); print("]\n");
                print("[s5: len="); print_uint(len); print("]\n");

                if (len > 512) len = 512;
                if (len == 0) { *ret = 0; break; }

                static char tmp[512];
                safe_copy_from_user(tmp, (const char *)data_ptr, len);
                tmp[len] = 0;
                print("[s5: data='"); print(tmp); print("']\n");

                int r = myfs_write(name, tmp, len);
                print("[s5: result="); print_uint((uint32_t)r); print("]\n");
                *ret = (r >= 0) ? (uint32_t)r : (uint32_t)-1;
            }
            break;

        case 6:  /* exec */
            {
                extern void exec_user_program(const char *name);
                char name[64];
                safe_copy_from_user(name, (const char *)arg, 64);
                exec_user_program(name);
            }
            break;

        case 7:  /* read_key: неблокирующий */
            if (keyboard_has_char()) {
                *ret = (uint32_t)(unsigned char)keyboard_get_char();
            } else {
                *ret = 0;
            }
            break;

        case 8:  /* cls: прямая очистка VGA-буфера */
            {
                volatile char *vga = (volatile char *)0xB8000;
                for (int i = 0; i < 80 * 25; i++) {
                    vga[i * 2] = ' ';
                    vga[i * 2 + 1] = 0x0F;
                }
            }
            break;

        case 9:  /* gotoxy: игнорируем (не нужен для прямой записи) */
            break;

        case 10:  /* ticks */
            *ret = get_ticks();
            break;

        case 11:  /* putchar_at: arg = (y << 24) | (x << 16) | c */
            {
                uint32_t c = arg & 0xFFFF;
                uint32_t x = (arg >> 16) & 0xFF;
                uint32_t y = (arg >> 24) & 0xFF;
                volatile char *vga = (volatile char *)0xB8000;
                int pos = y * 80 + x;
                if (pos >= 0 && pos < 80 * 25) {
                    vga[pos * 2] = (char)c;
                    vga[pos * 2 + 1] = 0x0F;
                }
            }
            break;

        case 12:  /* sleep: arg = ms */
            {
                extern uint32_t get_ticks(void);
                uint32_t start = get_ticks();
                uint32_t target = arg / 10;
                if (target == 0) target = 1;
                while (get_ticks() - start < target) {
                    __asm__ volatile ("sti; pause; cli");
                }
            }
            break;

        
        
        default:
            break;
    }
}
