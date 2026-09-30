#include "shell.h"
#include "util.h"
#include "heap.h"
#include "virtio.h"
#include "myfs.h"
#include "task.h"
#include "dmesg.h"
#include <stdint.h>

extern void print(const char *str);
extern void putchar(char c);
extern void putchar_attr(char c, uint8_t attr);
extern void clear_screen(void);
extern void print_uint(uint32_t n);
extern uint32_t get_ticks(void);
extern int  vga_get_cursor(void);
extern void vga_set_cursor(int pos);
extern void vga_set_color(uint8_t color);

#define LINE_MAX 128
#define CLIP_MAX 128
#define PROMPT   "myos> "
#define ATTR_SEL 0x17

static uint8_t cur_attr = 0x0F;
static char line[LINE_MAX];
static int  line_len = 0, caret = 0;
static int  sel_start = -1, sel_end = -1;
static char clip[CLIP_MAX];
static int  clip_len = 0;
static int  line_start = 0, screen_len = 0;

/* less mode */
static int  less_active = 0;
static char less_content[4096];
static int  less_len = 0;
static int  less_offset = 0;

static int streq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}
static int starts_with(const char *s, const char *p) {
    while (*p) { if (*s++ != *p++) return 0; }
    return 1;
}
static uint32_t parse_uint(const char *s) {
    uint32_t n = 0;
    while (*s >= '0' && *s <= '9') { n = n*10 + (*s - '0'); s++; }
    return n;
}
static uint32_t parse_hex(const char *s) {
    uint32_t n = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (1) {
        char c = *s; int d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else break;
        n = n*16 + d; s++;
    }
    return n;
}

static void redraw_line(void) {
    int lo = sel_start, hi = sel_end;
    if (lo > hi) { int t = lo; lo = hi; hi = t; }
    if (lo < 0) { lo = -1; hi = -1; }
    for (int i = 0; i < line_len; i++) {
        uint8_t attr = (lo >= 0 && i >= lo && i < hi) ? ATTR_SEL : cur_attr;
        putchar_attr(line[i], attr);
    }
    screen_len = line_len;
    vga_set_cursor(line_start + caret);
}
static void clear_input_line(void) {
    vga_set_cursor(line_start);
    for (int i = 0; i < screen_len; i++) putchar(' ');
    vga_set_cursor(line_start);
    screen_len = 0;
}
static void do_copy(void) {
    int lo = sel_start, hi = sel_end;
    if (lo < 0 || hi < 0) return;
    if (lo > hi) { int t = lo; lo = hi; hi = t; }
    clip_len = 0;
    for (int i = lo; i < hi && clip_len < CLIP_MAX - 1; i++) clip[clip_len++] = line[i];
}
static void do_cut(void) {
    int lo = sel_start, hi = sel_end;
    if (lo < 0 || hi < 0) return;
    if (lo > hi) { int t = lo; lo = hi; hi = t; }
    do_copy();
    for (int i = hi; i < line_len; i++) line[i - (hi - lo)] = line[i];
    line_len -= (hi - lo); caret = lo;
    sel_start = sel_end = -1;
    clear_input_line(); redraw_line();
}
static void do_paste(void) {
    for (int i = 0; i < clip_len; i++) {
        if (line_len >= LINE_MAX - 1) break;
        for (int j = line_len; j > caret; j--) line[j] = line[j-1];
        line[caret] = clip[i]; line_len++; caret++;
    }
    sel_start = sel_end = -1;
    clear_input_line(); redraw_line();
}
static void ensure_anchor(void) {
    if (sel_start < 0) { sel_start = caret; sel_end = caret; }
}
static void sel_left(void) {
    ensure_anchor();
    if (caret == 0) return;
    caret--;
    if (caret < sel_start) {
        sel_start = caret;
    } else {
        sel_end = caret;
    }
}
static void sel_right(void) {
    ensure_anchor();
    if (caret >= line_len) return;
    caret++;
    if (caret > sel_end) {
        sel_end = caret;
    } else {
        sel_start = caret;
    }
}

static void print_name(const char *name, uint32_t size) {
    print("\n  ");
    print(name);
    print("  (");
    print_uint(size);
    print(" bytes)");
}

static void cmd_help(void) {
    print("\nCommands: help clear ticks echo reboot uptime date hex mem gotoxy color");
    print("\n          meminfo malloc free heap disk format mount ls cat write rm df");
    print("\n          spawn tasks kill");
}
static void cmd_reboot(void) {
    __asm__ volatile ("cli");
    __asm__ volatile ("lidt (0)");
    __asm__ volatile ("int $0");
}
static void cmd_uptime(void) {
    print("\nUptime: "); print_uint(get_ticks()/100);
    print(" s ("); print_uint(get_ticks()); print(" ticks)");
}
static void cmd_date(void) {
    uint8_t h, m, s, d, mo; uint16_t y;
    rtc_read(&h,&m,&s,&d,&mo,&y);
    print("\nDate: ");
    if (d<10) putchar('0'); print_uint(d); putchar('.');
    if (mo<10) putchar('0'); print_uint(mo); putchar('.');
    print_uint(y); print(" ");
    if (h<10) putchar('0'); print_uint(h); putchar(':');
    if (m<10) putchar('0'); print_uint(m); putchar(':');
    if (s<10) putchar('0'); print_uint(s);
}
static void cmd_hex(const char *a) { print("\n0x"); hex_print(parse_hex(a)); }
static void cmd_mem(const char *a) {
    uint32_t addr = parse_hex(a);
    print("\n"); hex_print(addr); print(": "); hex_dump(addr, 16);
}
static void cmd_gotoxy(const char *a) {
    uint32_t x = parse_uint(a);
    while (*a && *a != ' ') a++;
    while (*a == ' ') a++;
    uint32_t y = parse_uint(a);
    vga_set_cursor(y * 80 + x);
}
static void cmd_color(const char *a) {
    uint32_t c = parse_uint(a); if (c > 15) c = 15;
    cur_attr = (uint8_t)c; vga_set_color(c);
}
static void cmd_meminfo(void) {
    print("\nHeap start: 0x"); hex_print(heap_start_addr());
    print("\nTotal: "); print_uint(heap_total());
    print(" bytes\nUsed:  "); print_uint(heap_used());
    print(" bytes\nFree:  "); print_uint(heap_free()); print(" bytes");
}
static void cmd_malloc(const char *a) {
    uint32_t n = parse_uint(a);
    void *p = kmalloc(n);
    if (!p) { print("\nOut of memory"); return; }
    print("\nAllocated "); print_uint(n); print(" bytes at 0x");
    hex_print((uint32_t)p);
}
static void cmd_free(const char *a) {
    uint32_t addr = parse_hex(a);
    kfree((void *)addr);
    print("\nFreed 0x"); hex_print(addr);
}
static void cmd_heap(void) {
    print("\n"); hex_dump(heap_start_addr(), 32);
}
static void cmd_disk(void) {
    char model[41];
    uint32_t sec;
    virtio_drive_info(model, &sec);
    print("\nDrive: "); print(model);
    print("\nSectors: "); print_uint(sec);
    print(" ("); print_uint(sec / 2048); print(" MB)");
}
static void cmd_format(void) {
    if (myfs_format() < 0) { print("\nFormat failed"); return; }
    print("\nFormatted with myfs");
}
static void cmd_mount(void) {
    if (myfs_mount() < 0) { print("\nNo myfs found"); return; }
    print("\nmyfs mounted");
}
static void cmd_ls(void) {
    if (!myfs_is_mounted()) { print("\nNot mounted"); return; }
    myfs_list(print_name);
}
static void cmd_cat(const char *a) {
    if (!myfs_is_mounted()) { print("\nNot mounted"); return; }
    char buf[513];
    int n = myfs_read(a, buf, 512);
    if (n < 0) { print("\nNo such file"); return; }
    buf[n] = 0;
    print("\n"); print(buf);
}
static void cmd_write(const char *a) {
    if (!myfs_is_mounted()) { print("\nNot mounted"); return; }
    const char *name = a;
    while (*a && *a != ' ') a++;
    if (*a != ' ') { print("\nUsage: write <name> <text>"); return; }
    uint32_t name_len = a - name;
    a++;
    const char *data = a;
    uint32_t data_len = 0;
    while (data[data_len]) data_len++;
    char name_buf[32];
    if (name_len >= 31) name_len = 31;
    for (uint32_t i = 0; i < name_len; i++) name_buf[i] = name[i];
    name_buf[name_len] = 0;
    if (myfs_write(name_buf, data, data_len) < 0) { print("\nWrite failed"); return; }
    print("\nWrote "); print_uint(data_len); print(" bytes to "); print(name_buf);
}
static void cmd_rm(const char *a) {
    if (!myfs_is_mounted()) { print("\nNot mounted"); return; }
    if (myfs_delete(a) < 0) { print("\nNo such file"); return; }
    print("\nDeleted "); print(a);
}
static void cmd_df(void) {
    if (!myfs_is_mounted()) { print("\nNot mounted"); return; }
    print("\nTotal: "); print_uint(myfs_total()); print(" sectors");
    print("\nUsed:  "); print_uint(myfs_used());  print(" sectors");
    print("\nFree:  "); print_uint(myfs_free());  print(" sectors");
}


static void cmd_spawn(void) {
    extern int start_worker_task(void);
    int id = start_worker_task();
    if (id < 0) { print("\nNo free slots"); return; }
    print("\nSpawned task id="); print_uint(id);
    print(" (letter=");
    putchar('A' + (id % 26));
    print(")");
}

static void cmd_tasks(void) {
    int n = task_count();
    print("\nTasks: "); print_uint(n);
    for (int i = 0; i < 8; i++) {
        task_t *t = task_get(i);
        if (!t || t->state == 0) continue;
        print("\n  id="); print_uint(t->id);
        print(" name="); print(t->name);
        print(" state=");
        if (t->state == 1) print("ready");
        else if (t->state == 2) print("running");
        else if (t->state == 3) print("dead");
        else print("?");
    }
}

static void cmd_kill(const char *a) {
    uint32_t id = parse_uint(a);
    if (id >= 8) { print("\nBad id"); return; }
    task_t *t = task_get(id);
    if (!t || t->state == 0) { print("\nNo such task"); return; }
    t->state = 3;
    print("\nKilled task "); print_uint(id);
}

/* --- less --- */
#define LESS_PAGE 20

static void less_status(const char *tag) {
    print("\n--- ");
    print(tag);
    print(": lines ");
    int cur_line = 1;
    for (int i = 0; i < less_offset && i < less_len; i++) {
        if (less_content[i] == '\n') cur_line++;
    }
    print_uint(cur_line);
    print(" / ");
    int total = 1;
    for (int i = 0; i < less_len; i++) if (less_content[i] == '\n') total++;
    print_uint(total);
    print("  Space=next  Enter=line  b=back  g=top  G=end  q=quit ---");
}

static void less_draw(void) {
    clear_screen();
    int shown = 0;
    int i = less_offset;
    while (i < less_len && shown < LESS_PAGE) {
        putchar(less_content[i]);
        if (less_content[i] == '\n') shown++;
        i++;
    }
    less_status("less");
}

static void less_start(const char *data, int len) {
    if (len > (int)sizeof(less_content) - 1) len = sizeof(less_content) - 1;
    for (int i = 0; i < len; i++) less_content[i] = data[i];
    less_content[len] = 0;
    less_len = len;
    less_offset = 0;
    less_active = 1;
    less_draw();
}

static void less_page_down(void) {
    int shown = 0;
    while (less_offset < less_len && shown < LESS_PAGE) {
        if (less_content[less_offset] == '\n') shown++;
        less_offset++;
    }
    less_draw();
}

static void less_line_down(void) {
    while (less_offset < less_len && less_content[less_offset] != '\n') less_offset++;
    if (less_offset < less_len) less_offset++;
    less_draw();
}

static void less_page_up(void) {
    int count = 0;
    while (less_offset > 0 && count < LESS_PAGE) {
        less_offset--;
        if (less_content[less_offset] == '\n') count++;
    }
    less_draw();
}

static void less_goto_top(void) {
    less_offset = 0;
    less_draw();
}

static void less_goto_end(void) {
    less_offset = 0;
    int shown = 0;
    int last = 0;
    int i = 0;
    while (i < less_len) {
        if (less_content[i] == '\n') shown++;
        if (shown > LESS_PAGE) {
            last++;
        }
        i++;
    }
    /* Идём к последней странице */
    int total_lines = 1;
    for (int k = 0; k < less_len; k++) if (less_content[k] == '\n') total_lines++;
    int target_line = total_lines - LESS_PAGE;
    if (target_line < 1) target_line = 1;
    int line = 1;
    int off = 0;
    while (off < less_len && line < target_line) {
        if (less_content[off] == '\n') line++;
        off++;
    }
    less_offset = off;
    less_draw();
}

static void less_quit(void) {
    less_active = 0;
    clear_screen();
    print("myos> ");
    line_start = vga_get_cursor();
    screen_len = 0;
    line_len = 0;
    caret = 0;
    sel_start = sel_end = -1;
}

static void cmd_kbd(void) {
    extern uint32_t keyboard_total_irq(void);
    extern uint32_t keyboard_total_pushed(void);
    extern uint32_t keyboard_total_special(void);
    print("\nIRQ1 count:    "); print_uint(keyboard_total_irq());
    print("\nChars pushed:  "); print_uint(keyboard_total_pushed());
    print("\nSpecial (←/→): "); print_uint(keyboard_total_special());
}

extern void print_vga(const char *str);

static void cmd_dmesg(void) {
    const char *b = dmesg_buf();
    print_vga("\n--- dmesg (");
    char tmp[16]; int n = dmesg_len(); int i = 0;
    if (n == 0) { tmp[i++] = '0'; }
    else while (n) { tmp[i++] = '0' + (n % 10); n /= 10; }
    while (i--) putchar(tmp[i]);
    print_vga(" bytes) ---\n");
    print_vga(b);
    print_vga("\n--- end ---");
}

static void cmd_dmesg_clear(void) {
    dmesg_clear();
    print("\ndmesg cleared");
}

static void cmd_append(const char *a) {
    if (!myfs_is_mounted()) { print("\nNot mounted"); return; }
    const char *name = a;
    while (*a && *a != ' ') a++;
    if (*a != ' ') { print("\nUsage: append <name> <text>"); return; }
    uint32_t name_len = a - name;
    a++;
    const char *data = a;
    uint32_t data_len = 0;
    while (data[data_len]) data_len++;
    char name_buf[32];
    if (name_len >= 31) name_len = 31;
    for (uint32_t i = 0; i < name_len; i++) name_buf[i] = name[i];
    name_buf[name_len] = 0;
    int r = myfs_append(name_buf, data, data_len);
    if (r < 0) { print("\nAppend failed"); return; }
    print("\nAppended "); print_uint(data_len);
    print(" bytes to "); print(name_buf);
    print(" (total "); print_uint((uint32_t)r); print(")");
}

static void cmd_layout(void) {
    extern int keyboard_is_ru(void);
    extern int keyboard_shift_down(void);
    extern int keyboard_ctrl_down(void);
    print("\nLayout: ");
    print(keyboard_is_ru() ? "RU" : "EN");
    print("\nShift:  ");
    print(keyboard_shift_down() ? "down" : "up");
    print("\nCtrl:   ");
    print(keyboard_ctrl_down() ? "down" : "up");
}

static void cmd_run(const char *a) {
    extern int run_user_program(const char *name);
    run_user_program(a);
}

static void cmd_install(void) {
//     extern int install_builtin_programs(void);
//     install_builtin_programs();
}

static void cmd_box(void) {
    extern void gui_init(void);
    extern void gui_draw(void);
    gui_init();
    gui_draw();
}

static void run_command(void) {
    if (line_len == 0) return;
    if (streq(line, "help")) cmd_help();
    else if (streq(line, "clear")) clear_screen();
    else if (streq(line, "ticks")) { print("\nTicks: "); print_uint(get_ticks()); }
    else if (streq(line, "reboot")) cmd_reboot();
    else if (streq(line, "uptime")) cmd_uptime();
    else if (streq(line, "date")) cmd_date();
    else if (streq(line, "meminfo")) cmd_meminfo();
    else if (streq(line, "heap")) cmd_heap();
    else if (streq(line, "disk")) cmd_disk();
    else if (streq(line, "format")) cmd_format();
    else if (streq(line, "mount")) cmd_mount();
    else if (streq(line, "ls")) cmd_ls();
    else if (streq(line, "df")) cmd_df();
    else if (streq(line, "box")) cmd_box();
    else if (streq(line, "gui")) cmd_box();
    else if (streq(line, "spawn")) cmd_spawn();
    else if (streq(line, "tasks")) cmd_tasks();
    else if (starts_with(line, "run ")) cmd_run(line + 4);
    else if (streq(line, "install")) cmd_install();
    else if (streq(line, "kbd")) cmd_kbd();
    else if (streq(line, "dmesg")) cmd_dmesg();
    else if (streq(line, "layout")) cmd_layout();
    else if (streq(line, "dmesg clear")) cmd_dmesg_clear();
    else if (starts_with(line, "hex ")) cmd_hex(line + 4);
    else if (starts_with(line, "mem ")) cmd_mem(line + 4);
    else if (starts_with(line, "gotoxy ")) cmd_gotoxy(line + 7);
    else if (starts_with(line, "color ")) cmd_color(line + 6);
    else if (starts_with(line, "malloc ")) cmd_malloc(line + 7);
    else if (starts_with(line, "free ")) cmd_free(line + 5);
    else if (starts_with(line, "cat ")) cmd_cat(line + 4);
    else if (starts_with(line, "less ")) {
        extern int myfs_is_mounted(void);
        extern int myfs_read(const char *name, char *buf, uint32_t max);
        if (!myfs_is_mounted()) { print("\nNot mounted"); return; }
        const char *fname = line + 5;
        char buf[4095];
        int n = myfs_read(fname, buf, 4094);
        if (n < 0) { print("\nNo such file"); return; }
        buf[n] = 0;
        less_start(buf, n);
        return;
    }
    else if (starts_with(line, "write ")) cmd_write(line + 6);
    else if (starts_with(line, "append ")) cmd_append(line + 7);
    else if (starts_with(line, "rm ")) cmd_rm(line + 3);
    else if (starts_with(line, "kill ")) cmd_kill(line + 5);
    else if (starts_with(line, "echo ")) {
        print("\n");
        for (int i = 5; i < line_len; i++) putchar(line[i]);
    } else {
        print("\nUnknown command: ");
        for (int i = 0; i < line_len; i++) putchar(line[i]);
    }
}

void shell_init(void) {
    line_len = 0; caret = 0;
    sel_start = sel_end = -1; clip_len = 0;
    print("\n" PROMPT);
    line_start = vga_get_cursor(); screen_len = 0;
}
extern int keyboard_is_ru(void);

void shell_handle_char(char c) {
    /* переключение раскладки */
    if (c == 0x1F) {
        /* показать индикатор EN/RU в углу */
        volatile char *vga = (volatile char *)0xB8000;
        int pos = 79;   /* правый верхний угол */
        if (keyboard_is_ru()) {
            vga[pos*2] = 'U'; vga[pos*2+1] = 0x0E;
        } else {
            vga[pos*2] = 'E'; vga[pos*2+1] = 0x0E;
        }
        return;
    }
    if (less_active) {
        if (c == 'q' || c == 'Q') { less_quit(); return; }
        if (c == ' ') { less_page_down(); return; }
        if (c == '\n') { less_line_down(); return; }
        if (c == 'b' || c == 'B') { less_page_up(); return; }
        if (c == 'g') { less_goto_top(); return; }
        if (c == 'G') { less_goto_end(); return; }
        return;
    }
    if (c == '\n') {
        line[line_len] = 0; sel_start = sel_end = -1;
        run_command();
        line_len = 0; caret = 0;
        print("\n" PROMPT);
        line_start = vga_get_cursor(); screen_len = 0;
        return;
    }
    if (c == '\b') {
        if (caret > 0) {
            for (int i = caret - 1; i < line_len - 1; i++) line[i] = line[i+1];
            line_len--; caret--;
            sel_start = sel_end = -1;
            clear_input_line(); redraw_line();
        }
        return;
    }
    if (c == 0x11) { sel_left();  clear_input_line(); redraw_line(); return; }
    if (c == 0x12) { sel_right(); clear_input_line(); redraw_line(); return; }
    if (c == 0x13) { ensure_anchor(); sel_start = 0; caret = 0; clear_input_line(); redraw_line(); return; }
    if (c == 0x14) { ensure_anchor(); sel_end = line_len; caret = line_len; clear_input_line(); redraw_line(); return; }
    if (c == 0x1B) { sel_start = sel_end = -1; clear_input_line(); redraw_line(); return; }
    if (c == 0x03) { do_copy();  return; }
    if (c == 0x16) { do_paste(); return; }
    if (c == 0x18) { do_cut();   return; }
    if (c == 0x0C) { clear_screen(); print(PROMPT); line_start = vga_get_cursor(); screen_len = 0; redraw_line(); return; }
    {
        unsigned char uc = (unsigned char)c;
        if (uc >= 32 && uc != 127) {
            if (line_len < LINE_MAX - 1) {
                for (int i = line_len; i > caret; i--) line[i] = line[i-1];
                line[caret] = c; line_len++; caret++;
                sel_start = sel_end = -1;
                clear_input_line(); redraw_line();
            }
        }
    }
}
void shell_tick(void) { }
