#include "keyboard.h"
#include "io.h"

#define KBD_DATA 0x60
#define BUF_SIZE 1024

static const char kbd_en_lower[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',
    0,  '*', 0,  ' ',
};
static const char kbd_en_upper[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?',
    0,  '*', 0,  ' ',
};

static char buf[BUF_SIZE];
static volatile int head = 0;
static volatile int tail = 0;
static int ctrl_down = 0;
static int shift_down = 0;
static int extended = 0;

void keyboard_init(void) {
    head = 0; tail = 0;
    ctrl_down = 0; shift_down = 0; extended = 0;
}
static void push_char(char c) {
    int next = (head + 1) % BUF_SIZE;
    if (next != tail) { buf[head] = c; head = next; }
}

#define K_LEFT   0x11
#define K_RIGHT  0x12
#define K_HOME   0x13
#define K_END    0x14
#define K_ESC    0x1B
#define K_CTRL_C 0x03
#define K_CTRL_V 0x16
#define K_CTRL_X 0x18
#define K_CTRL_L 0x0C
#define K_PGUP   0x15
#define K_PGDN   0x17

void keyboard_handler(void) {
    uint8_t sc = inb(KBD_DATA);
    if (sc == 0xE0) { extended = 1; return; }
    if (sc & 0x80) {
        uint8_t rel = sc & 0x7F;
        if (rel == 0x1D) ctrl_down = 0;
        if (rel == 0x2A || rel == 0x36) shift_down = 0;
        extended = 0;
        return;
    }
    if (sc == 0x1D) { ctrl_down = 1; return; }
    if (sc == 0x2A || sc == 0x36) { shift_down = 1; return; }

    if (extended) {
        extended = 0;
        if (sc == 0x4B) { push_char(K_LEFT);  return; }
        if (sc == 0x4D) { push_char(K_RIGHT); return; }
        if (sc == 0x47) { push_char(K_HOME);  return; }
        if (sc == 0x4F) { push_char(K_END);   return; }
        if (sc == 0x49) { push_char(K_PGUP);  return; }
        if (sc == 0x51) { push_char(K_PGDN);  return; }
        return;
    }
    if (sc == 0x01) { push_char(K_ESC); return; }
    if (sc >= 128) return;

    char c = shift_down ? kbd_en_upper[sc] : kbd_en_lower[sc];
    if (!c) return;

    if (ctrl_down) {
        if (c == 'c' || c == 'C') { push_char(K_CTRL_C); return; }
        if (c == 'v' || c == 'V') { push_char(K_CTRL_V); return; }
        if (c == 'x' || c == 'X') { push_char(K_CTRL_X); return; }
        if (c == 'l' || c == 'L') { push_char(K_CTRL_L); return; }
        return;
    }
    push_char(c);
}

int keyboard_has_char(void) { return head != tail; }
char keyboard_get_char(void) {
    if (head == tail) return 0;
    char c = buf[tail];
    tail = (tail + 1) % BUF_SIZE;
    return c;
}

int keyboard_is_ru(void) { return 0; }
int keyboard_shift_down(void) { return shift_down; }
int keyboard_ctrl_down(void) { return ctrl_down; }
uint32_t keyboard_total_irq(void) { return 0; }
uint32_t keyboard_total_pushed(void) { return 0; }
uint32_t keyboard_total_special(void) { return 0; }
