#ifndef MOUSE_H
#define MOUSE_H
#include <stdint.h>
void mouse_init(void);
void mouse_handler(void);
int  mouse_get_x(void);
int  mouse_get_y(void);
int  mouse_get_buttons(void);
int  mouse_moved(void);
void mouse_clear_moved(void);
void mouse_cursor_draw(void);
void mouse_cursor_hide(void);
#endif
