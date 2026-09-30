#ifndef TASK_H
#define TASK_H
#include <stdint.h>

#define TASK_STACK_SIZE 8192
#define MAX_TASKS 8

typedef struct {
    uint32_t esp;      /* сохранённый указатель стека */
    uint32_t id;
    int      state;    /* 0=free, 1=ready, 2=running, 3=dead */
    char     name[16];
} task_t;

void     task_init(void);
int      task_create(const char *name, void (*entry)(void));
void     task_switch(void);
task_t  *task_current(void);
int      task_count(void);
task_t  *task_get(int i);
uint32_t task_ticks(void);

#endif

void task_yield(void);
int  start_worker_task(void);
