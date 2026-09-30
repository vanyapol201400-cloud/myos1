#include "gdt.h"
#include <stdint.h>

extern void print(const char *str);
extern void print_uint(uint32_t n);

/* GDT: null, kernel code, kernel data, user code, user data, TSS */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* TSS */
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1, ss1, esp2, ss2;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct gdt_entry gdt[6];
static struct gdt_ptr   gp;
static struct tss_entry tss;

extern void gdt_flush(uint32_t gp_ptr);
extern void tss_flush(void);

static void gdt_set_gate(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[i].base_low  = base & 0xFFFF;
    gdt[i].base_mid  = (base >> 16) & 0xFF;
    gdt[i].base_high = (base >> 24) & 0xFF;
    gdt[i].limit_low = limit & 0xFFFF;
    gdt[i].gran      = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access    = access;
}

static void tss_init(void) {
    uint8_t *p = (uint8_t *)&tss;
    for (uint32_t i = 0; i < sizeof(tss); i++) p[i] = 0;
    tss.ss0 = 0x10;              /* kernel data segment */
    tss.esp0 = 0x200000;   /* 2 МБ */          /* kernel stack (стартовое значение) */
    tss.iomap_base = sizeof(tss);
}

void gdt_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}

void gdt_init(void) {
    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0,          0,    0);     /* null */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);  /* kernel code */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);  /* kernel data */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);  /* user code */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);  /* user data */

    tss_init();
    gdt_set_gate(5, (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x00);

    gdt_flush((uint32_t)&gp);
    tss_flush();

    print("GDT + TSS initialized\n");
}
