#include "task.h"
#include <stdint.h>

extern void print(const char *str);
extern void print_uint(uint32_t n);
extern void putchar(char c);

extern void task_switch_asm(uint32_t *old_esp, uint32_t new_esp);

static task_t tasks[MAX_TASKS];
static int    current = -1;
static uint32_t ticks = 0;

/* Стеки задач (статические, чтобы не возиться с kmalloc) */
static uint8_t stacks[MAX_TASKS][TASK_STACK_SIZE] __attribute__((aligned(16)));

void task_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].esp = 0;
        tasks[i].id = i;
        tasks[i].state = 0;
        tasks[i].name[0] = 0;
    }
    current = -1;
    ticks = 0;
}

int task_create(const char *name, void (*entry)(void)) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == 0) {
            tasks[i].state = 1;
            tasks[i].id = i;

            int k = 0;
            while (name[k] && k < 15) { tasks[i].name[k] = name[k]; k++; }
            tasks[i].name[k] = 0;

            /* Настраиваем стек. Верх стека — конец массива (растёт вниз). */
            uint32_t *sp = (uint32_t *)(stacks[i] + TASK_STACK_SIZE);
            sp = (uint32_t *)((uint32_t)sp & ~0xF);   /* выравнивание 16 */

            /* Кладём на стек то, что ожидает наша switch-функция:
               [eflags] [cs] [eip] [edi] [esi] [ebp] [ebx] [edx] [ecx] [eax]
               Мы используем "iret"-подобный возврат в задачу. */

            /* return address для ret в task_switch_asm */
            *(--sp) = (uint32_t)entry;   /* eip — куда вернуться */

            /* Регистры для popa (порядок: eax, ecx, edx, ebx, esp, ebp, esi, edi) */
            *(--sp) = 0;   /* edi */
            *(--sp) = 0;   /* esi */
            *(--sp) = 0;   /* ebp */
            *(--sp) = 0;   /* esp (не используется) */
            *(--sp) = 0;   /* ebx */
            *(--sp) = 0;   /* edx */
            *(--sp) = 0;   /* ecx */
            *(--sp) = 0;   /* eax */

            tasks[i].esp = (uint32_t)sp;
            return i;
        }
    }
    return -1;
}

task_t *task_current(void) {
    if (current < 0) return 0;
    return &tasks[current];
}

int task_count(void) {
    int n = 0;
    for (int i = 0; i < MAX_TASKS; i++) if (tasks[i].state != 0) n++;
    return n;
}

task_t *task_get(int i) {
    if (i < 0 || i >= MAX_TASKS) return 0;
    return &tasks[i];
}

uint32_t task_ticks(void) { return ticks; }

void task_switch(void) {
    if (current < 0) return;

    /* Находим следующую готовую задачу */
    int next = -1;
    for (int i = 1; i <= MAX_TASKS; i++) {
        int idx = (current + i) % MAX_TASKS;
        if (tasks[idx].state == 1 || tasks[idx].state == 2) {
            next = idx;
            break;
        }
    }
    if (next < 0 || next == current) return;

    tasks[current].state = 1;   /* была running -> ready */
    tasks[next].state = 2;      /* становится running */

    int prev = current;
    current = next;
    ticks++;

    /* Переключаемся */
    task_switch_asm(&tasks[prev].esp, tasks[next].esp);
}

/* Первый запуск задачи — переключение с фиктивного контекста ядра */
extern void task_switch_asm(uint32_t *old_esp, uint32_t new_esp);

void task_start_first(void) {
    if (current == -1) {
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state == 1 || tasks[i].state == 2) {
                current = i;
                tasks[i].state = 2;
                uint32_t dummy;
                task_switch_asm(&dummy, tasks[i].esp);
                return;
            }
        }
    }
}

void task_yield(void) {
    /* Просто вызываем task_switch — текущая задача переходит в ready,
       следующая становится running. */
    extern void task_switch(void);
    task_switch();
}
