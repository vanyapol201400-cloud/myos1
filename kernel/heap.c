#include "heap.h"
#include <stdint.h>

#define HEAP_START 0x800000    /* 8 МБ */
#define HEAP_SIZE  0x400000    /* 4 МБ до 12 МБ */

typedef struct block {
    uint32_t size;
    uint32_t free;
    struct block *next;
} block_t;

#define HDR (sizeof(block_t))

static block_t *head = 0;
static uint32_t total = 0;
static uint32_t used = 0;

void heap_init(void) {
    head = (block_t *)HEAP_START;
    head->size = HEAP_SIZE - HDR;
    head->free = 1;
    head->next = 0;
    total = HEAP_SIZE;
    used = 0;
}

static void split(block_t *b, uint32_t size) {
    if (b->size <= size + HDR + 16) return;
    block_t *nb = (block_t *)((uint8_t *)b + HDR + size);
    nb->size = b->size - size - HDR;
    nb->free = 1;
    nb->next = b->next;
    b->size = size;
    b->next = nb;
}

void *kmalloc(uint32_t size) {
    if (size == 0) return 0;
    size = (size + 7) & ~7u;
    block_t *b = head;
    while (b) {
        if (b->free && b->size >= size) {
            split(b, size);
            b->free = 0;
            used += b->size + HDR;
            return (void *)((uint8_t *)b + HDR);
        }
        b = b->next;
    }
    return 0;
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_t *b = (block_t *)((uint8_t *)ptr - HDR);
    b->free = 1;
    used -= b->size + HDR;
    b = head;
    while (b) {
        if (b->free && b->next && b->next->free) {
            b->size += HDR + b->next->size;
            b->next = b->next->next;
            continue;
        }
        b = b->next;
    }
}

uint32_t heap_total(void) { return total; }
uint32_t heap_used(void)  { return used; }
uint32_t heap_free(void)  { return total - used; }
uint32_t heap_start_addr(void) { return HEAP_START; }
