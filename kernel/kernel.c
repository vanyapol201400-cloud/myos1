#include <stdint.h>
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "mouse.h"
#include "shell.h"
#include "paging.h"
#include "heap.h"
#include "virtio.h"
#include "gdt.h"
#include "dmesg.h"
#include "io.h"

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define SCROLL_LINES 200

static uint32_t ticks = 0;
static uint8_t attr = 0x0F;

static char    sb_chars[SCROLL_LINES][VGA_WIDTH];
static uint8_t sb_attrs[SCROLL_LINES][VGA_WIDTH];
static int     sb_count = 0;
static int     sb_cursor_row = 0;
static int     sb_cursor_col = 0;
static int     scroll_offset = 0;

static void vga_set_cursor_hw(int pos) {
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void render(void) {
    volatile char *vga = (volatile char *)VGA_MEMORY;
    int first = sb_count - VGA_HEIGHT - scroll_offset;
    if (first < 0) first = 0;
    for (int row = 0; row < VGA_HEIGHT; row++) {
        int src = first + row;
        for (int col = 0; col < VGA_WIDTH; col++) {
            char c = ' ';
            uint8_t a = attr;
            if (src < sb_count && src >= 0) {
                c = sb_chars[src][col];
                a = sb_attrs[src][col];
            }
            vga[(row * VGA_WIDTH + col) * 2] = c;
            vga[(row * VGA_WIDTH + col) * 2 + 1] = a;
        }
    }
    if (scroll_offset == 0) {
        int vis_row = sb_cursor_row - first;
        if (vis_row >= 0 && vis_row < VGA_HEIGHT) {
            vga_set_cursor_hw(vis_row * VGA_WIDTH + sb_cursor_col);
        } else {
            vga_set_cursor_hw(VGA_HEIGHT * VGA_WIDTH - 1);
        }
    }
}


void vga_put_char_at(int x, int y, char c, uint8_t attr) {
    volatile char *vga = (volatile char *)VGA_MEMORY;
    int pos = y * VGA_WIDTH + x;
    if (pos >= 0 && pos < VGA_WIDTH * VGA_HEIGHT) {
        vga[pos * 2] = c;
        vga[pos * 2 + 1] = attr;
    }
}

void cls_direct(void) {
    volatile char *vga = (volatile char *)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i * 2] = ' ';
        vga[i * 2 + 1] = 0x07;
    }
    /* синхронизировать sb_* с VGA */
    for (int i = 0; i < SCROLL_LINES; i++)
        for (int j = 0; j < VGA_WIDTH; j++) {
            sb_chars[i][j] = ' ';
            sb_attrs[i][j] = 0x07;
        }
    sb_count = 0;
    sb_cursor_row = 0;
    sb_cursor_col = 0;
    scroll_offset = 0;
    render();
}

static void sb_newline(void) {
    sb_cursor_col = 0;
    sb_cursor_row++;
    if (sb_cursor_row >= SCROLL_LINES) {
        for (int i = 1; i < SCROLL_LINES; i++)
            for (int j = 0; j < VGA_WIDTH; j++) {
                sb_chars[i-1][j] = sb_chars[i][j];
                sb_attrs[i-1][j] = sb_attrs[i][j];
            }
        for (int j = 0; j < VGA_WIDTH; j++) {
            sb_chars[SCROLL_LINES-1][j] = ' ';
            sb_attrs[SCROLL_LINES-1][j] = attr;
        }
        sb_cursor_row = SCROLL_LINES - 1;
    }
    if (sb_cursor_row + 1 > sb_count) sb_count = sb_cursor_row + 1;
}

void putchar_attr(char c, uint8_t a) {
    if (c == '\n') { sb_newline(); render(); return; }
    if (c == '\r') { sb_cursor_col = 0; render(); return; }
    if (c == '\b') {
        if (sb_cursor_col > 0) {
            sb_cursor_col--;
            sb_chars[sb_cursor_row][sb_cursor_col] = ' ';
            sb_attrs[sb_cursor_row][sb_cursor_col] = attr;
            render();
        }
        return;
    }
    if (sb_cursor_col >= VGA_WIDTH) sb_newline();
    if (sb_cursor_row >= SCROLL_LINES) return;
    sb_chars[sb_cursor_row][sb_cursor_col] = c;
    sb_attrs[sb_cursor_row][sb_cursor_col] = a;
    sb_cursor_col++;
    if (sb_cursor_row + 1 > sb_count) sb_count = sb_cursor_row + 1;
    scroll_offset = 0;
    render();
}
void putchar(char c) { putchar_attr(c, attr); }
void print_vga(const char *str) { while (*str) putchar(*str++); }
void print(const char *str) {
    while (*str) { putchar(*str); dmesg_putchar(*str); str++; }
}
void print_uint(uint32_t n) {
    if (n == 0) { putchar('0'); return; }
    char tmp[16]; int i = 0;
    while (n) { tmp[i++] = '0' + (n % 10); n /= 10; }
    while (i--) putchar(tmp[i]);
}
void print_hex8(uint8_t n) {
    const char *h = "0123456789ABCDEF";
    putchar(h[(n >> 4) & 0xF]); putchar(h[n & 0xF]);
}
void print_hex32(uint32_t n) {
    const char *h = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) putchar(h[(n >> i) & 0xF]);
}
void clear_screen(void) {
    for (int i = 0; i < SCROLL_LINES; i++)
        for (int j = 0; j < VGA_WIDTH; j++) {
            sb_chars[i][j] = ' ';
            sb_attrs[i][j] = attr;
        }
    sb_count = 0; sb_cursor_row = 0; sb_cursor_col = 0; scroll_offset = 0;
    render();
}
uint32_t get_ticks(void) { return ticks; }
int  vga_get_cursor(void) { return sb_cursor_row * VGA_WIDTH + sb_cursor_col; }
void vga_set_cursor(int pos) {
    sb_cursor_row = pos / VGA_WIDTH;
    sb_cursor_col = pos % VGA_WIDTH;
    render();
}
void vga_set_color(uint8_t c) { attr = c; }
void scroll_up(int n) {
    int max_scroll = sb_count - VGA_HEIGHT;
    if (max_scroll < 0) max_scroll = 0;
    scroll_offset += n;
    if (scroll_offset > max_scroll) scroll_offset = max_scroll;
    render();
}
void scroll_down(int n) {
    scroll_offset -= n;
    if (scroll_offset < 0) scroll_offset = 0;
    render();
}
void scroll_to_end(void) { scroll_offset = 0; render(); }

static const char *exc_names[] = {
    "Divide by zero","Debug","NMI","Breakpoint","Overflow","Bound range",
    "Invalid opcode","Device not available","Double fault","Coprocessor overrun",
    "Invalid TSS","Segment not present","Stack fault","General protection",
    "Page fault","Reserved"
};
struct regs {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

void isr_handler(struct regs *r) {
    if (r->int_no < 32) {
        print("\nEXCEPTION: "); print(exc_names[r->int_no]);
        print("\nerr_code=0x"); print_hex32(r->err_code);
        print(" eip=0x"); print_hex32(r->eip);
        print(" cs=0x"); print_hex32(r->cs);
        print(" eflags=0x"); print_hex32(r->eflags);
        if (r->int_no == 14) {
            uint32_t cr2;
            __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
            print("\nFaulting address: 0x"); print_hex32(cr2);
        }
        print("\nSystem halted.\n");
        while (1) __asm__ volatile ("hlt");
    } else if (r->int_no == 32) {
        ticks++; pic_eoi(0);
    } else if (r->int_no == 33) {
        keyboard_handler(); pic_eoi(1);
    } else if (r->int_no == 44) {
        mouse_handler();
        pic_eoi(12);
    } else if (r->int_no == 128) {
        extern void syscall_dispatch(uint32_t num, uint32_t arg, uint32_t *ret);
        uint32_t ret = 0;
        syscall_dispatch(r->eax, r->ebx, &ret);
        r->eax = ret;
    }
}
void timer_init(void) {
    uint32_t divisor = 1193182 / 100;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

extern void shell_loop(void);

void kernel_main(void) {
    dmesg_init();
    clear_screen();
    print("Hello from my kernel!\n");
    print("Boot OK.\n");

    gdt_init();
    pic_init();
    idt_init();
    timer_init();
    keyboard_init();
    mouse_init();
    print("Mouse initialized\n");

    heap_init();
    print("Heap initialized.\n");
    paging_init();
    print("Paging enabled.\n");

    extern void pci_scan_all_1af4(void);
    pci_scan_all_1af4();
    if (virtio_init() == 0) print("virtio OK\n");
    else print("virtio failed\n");

    {
        extern int virtio_net_init(void);
        extern void virtio_net_get_mac(uint8_t *mac);
        int r = virtio_net_init();
        if (r == 0) {
            print("virtio-net OK, MAC: ");
            uint8_t mac[6];
            virtio_net_get_mac(mac);
            const char *hex = "0123456789ABCDEF";
            for (int i = 0; i < 6; i++) {
                putchar(hex[(mac[i] >> 4) & 0xF]);
                putchar(hex[mac[i] & 0xF]);
                if (i < 5) putchar(':');
            }
            print("\n");
        } else {
            print("virtio-net failed\n");
        }
    }

    print("Scroll: Shift+PgUp/PgDn, Esc=end\n");

    /* Автомонтирование и запуск user shell */
    {
        extern int myfs_mount(void);
        extern int myfs_is_mounted(void);
        extern void run_user_shell(void);
        int mr = myfs_mount();
        print("[mount r="); print_uint((uint32_t)mr);
        print(" is="); print_uint((uint32_t)myfs_is_mounted());
        print("]\n");
        if (mr == 0 && myfs_is_mounted()) {
            print("myfs auto-mounted\n");
            /* run_user_shell(); — отключено для отладки */
            /* никогда не вернёмся */
        } else {
            print("FS not mounted — starting kernel shell\n");
        }
    }

    extern void shell_init(void);
    shell_init();
    extern void gui_init(void);
    gui_init();
    extern void shell_loop(void);
    shell_loop();   /* вечный цикл: клавиатура + мышь */
}
