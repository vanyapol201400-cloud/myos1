#ifndef GUI_H
#define GUI_H
#include <stdint.h>
void gui_init(void);
void gui_draw(void);
void gui_key(int key);
void gui_handle_click(int mx, int my);
int  gui_active(void);
void gui_exit(void);
#endif
