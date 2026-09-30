/* Змейка — прямая работа с VGA-буфером через syscall 11 */
#define W 40
#define H 20
#define MAX_LEN 100

static void sys_print(const char *s) { __asm__ volatile ("int $0x80" : : "a"(2), "b"(s)); }
static void sys_putchar(char c) { __asm__ volatile ("int $0x80" : : "a"(1), "b"(c)); }
static void sys_exit(void) { __asm__ volatile ("int $0x80" : : "a"(0)); }
static int sys_read_key(void) { unsigned int ret; __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(7)); return (int)ret; }
static void sys_cls(void) { __asm__ volatile ("int $0x80" : : "a"(8)); }
static unsigned int sys_ticks(void) { unsigned int ret; __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(10)); return ret; }
static void sys_putchar_at(int x, int y, char c) {
    unsigned int arg = ((y & 0xFF) << 24) | ((x & 0xFF) << 16) | (c & 0xFFFF);
    __asm__ volatile ("int $0x80" : : "a"(11), "b"(arg));
}

static void print_num_at(int x, int y, int n) {
    if (n == 0) { sys_putchar_at(x, y, '0'); return; }
    char buf[12]; int i = 0;
    while (n) { buf[i++] = '0' + n % 10; n /= 10; }
    while (i--) { sys_putchar_at(x++, y, buf[i]); }
}

static void print_at(int x, int y, const char *s) {
    while (*s) { sys_putchar_at(x++, y, *s++); }
}

static unsigned int seed = 12345;
static unsigned int rnd(void) { seed = seed * 1103515245 + 12345; return (seed >> 16) & 0x7FFF; }

static int snake_x[MAX_LEN], snake_y[MAX_LEN];
static int snake_len, dir_x, dir_y, food_x, food_y, score, game_over;

static void spawn_food(void) {
    food_x = 1 + (rnd() % (W - 2));
    food_y = 1 + (rnd() % (H - 2));
}

static void draw(void) {
    sys_cls();
    for (int x = 0; x < W; x++) { sys_putchar_at(x, 0, '#'); sys_putchar_at(x, H - 1, '#'); }
    for (int y = 0; y < H; y++) { sys_putchar_at(0, y, '#'); sys_putchar_at(W - 1, y, '#'); }
    for (int i = 0; i < snake_len; i++) sys_putchar_at(snake_x[i], snake_y[i], i == 0 ? '@' : 'o');
    sys_putchar_at(food_x, food_y, '*');
    print_at(W + 2, 2, "Score: ");
    print_num_at(W + 9, 2, score);
}

static void init_game(void) {
    snake_len = 3;
    snake_x[0] = 10; snake_y[0] = 10;
    snake_x[1] = 9;  snake_y[1] = 10;
    snake_x[2] = 8;  snake_y[2] = 10;
    dir_x = 1; dir_y = 0;
    score = 0; game_over = 0;
    spawn_food();
}

static void game_step(void) {
    int nx = snake_x[0] + dir_x;
    int ny = snake_y[0] + dir_y;
    if (nx <= 0 || nx >= W - 1 || ny <= 0 || ny >= H - 1) { game_over = 1; return; }
    for (int i = 0; i < snake_len; i++) if (snake_x[i] == nx && snake_y[i] == ny) { game_over = 1; return; }
    if (nx == food_x && ny == food_y) { if (snake_len < MAX_LEN) snake_len++; score += 10; spawn_food(); }
    else { for (int i = snake_len - 1; i > 0; i--) { snake_x[i] = snake_x[i-1]; snake_y[i] = snake_y[i-1]; } }
    snake_x[0] = nx; snake_y[0] = ny;
}

void _start(void) __attribute__((section(".text._start")));
void _start(void) {
    init_game();
    draw();
    unsigned int last_tick = sys_ticks();
    while (!game_over) {
        int k = sys_read_key();
        if (k != 0) {
            if (k == 'w' && dir_y != 1) { dir_x = 0; dir_y = -1; }
            else if (k == 's' && dir_y != -1) { dir_x = 0; dir_y = 1; }
            else if (k == 'a' && dir_x != 1) { dir_x = -1; dir_y = 0; }
            else if (k == 'd' && dir_x != -1) { dir_x = 1; dir_y = 0; }
        }
        unsigned int now = sys_ticks();
        if (now - last_tick >= 15) { last_tick = now; game_step(); if (!game_over) draw(); }
    }
    sys_cls();
    print_at(10, 10, "GAME OVER! Score: ");
    print_num_at(28, 10, score);
    print_at(10, 12, "Press any key to exit...");
    while (sys_read_key() == 0) { }
    sys_exit();
}
