#include "task.h"
#include <stdint.h>

extern void putchar(char c);
extern void print(const char *str);
extern void print_uint(uint32_t n);

/* Каждая задача печатает свою букву в углу (позиция 78,0) */
static void task_worker(void) {
    task_t *self = task_current();
    if (!self) return;

    char letter = 'A' + (self->id % 26);

    while (self->state != 3) {
        /* Печатаем букву в углу через VGA-буфер напрямую */
        volatile char *vga = (volatile char *)0xB8000;
        int pos = 78;   /* правая колонка, верхняя строка */
        vga[pos * 2] = letter;
        vga[pos * 2 + 1] = 0x0E;   /* жёлтый на чёрном */

        /* Небольшая задержка */
        for (volatile int i = 0; i < 5000000; i++) { }

        /* Кооперативно отдаём управление */
        extern void task_yield(void);
        task_yield();
    }
}

void start_test_tasks(void) {
    task_init();
}

int start_worker_task(void) {
    return task_create("work", task_worker);
}
