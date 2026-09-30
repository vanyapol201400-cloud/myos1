#include "paging.h"

/* Page tables в 16 МБ */
#define PAGE_DIR    0x1000000
#define PAGE_TABLE  0x1001000

void paging_init(void) {
    uint32_t *pd = (uint32_t *)PAGE_DIR;
    for (int i = 0; i < 1024; i++) pd[i] = 0;

    /* Identity-map 0..24 МБ с флагом USER (7 = present|rw|user) */
    for (int t = 0; t < 8; t++) {
        uint32_t *pt = (uint32_t *)(PAGE_TABLE + t * 0x1000);
        for (int i = 0; i < 1024; i++) {
            pt[i] = ((t * 0x400000) + i * 0x1000) | 7;   /* USER! */
        }
        pd[t] = (PAGE_TABLE + t * 0x1000) | 7;            /* USER! */
    }

    __asm__ volatile ("mov %0, %%cr3" : : "r"(PAGE_DIR));

    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
}
