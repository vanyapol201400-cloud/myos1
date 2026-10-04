#include <stdint.h>

extern void shell_init(void);
extern void shell_handle_char(char c);
extern int  keyboard_has_char(void);
extern char keyboard_get_char(void);
extern void scroll_up(int);
extern void scroll_down(int);
extern void scroll_to_end(void);
extern void gui_init(void);
extern void gui_draw(void);
extern void gui_key(int key);
extern int  gui_active(void);
extern void gui_exit(void);
extern void cls_direct(void);
extern void draw_clock(int x, int y);
extern uint32_t get_ticks(void);

#define VGA_HEIGHT 25

void shell_loop(void) {
    uint32_t last_sec = 0;
    while (1) {
        __asm__ volatile ("hlt");

        /* Обновление часов каждую секунду */
        uint32_t now_sec = get_ticks() / 100;
        if (now_sec != last_sec) {
            last_sec = now_sec;
            if (gui_active()) {
                draw_clock(60, 0);
            }
        }

        if (keyboard_has_char()) {
            char c = keyboard_get_char();

            if (gui_active()) {
                gui_key(c);
                if (!gui_active()) {
                    cls_direct();
                    shell_init();
                }
            } else {
                if (c == 0x15) { scroll_up(VGA_HEIGHT - 2); }
                else if (c == 0x17) { scroll_down(VGA_HEIGHT - 2); }
                else if (c == 0x1B) { scroll_to_end(); }
                else {
                    scroll_to_end();
                    shell_handle_char(c);
                }
            }
        }
    }
}

void shell_restart(void) {
    __asm__ volatile ("sti");
    shell_init();
    shell_loop();
}
