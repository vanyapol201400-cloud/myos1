#include "idt.h"

#define IDT_ENTRIES 256

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr   idtp;

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr13(void);
extern void isr14(void);
extern void isr32(void);
extern void isr33(void);
extern void isr44(void);
extern void isr128(void);

static void idt_set_gate_flags(int n, uint32_t handler, uint8_t flags) {
    idt[n].base_low  = handler & 0xFFFF;
    idt[n].base_high = (handler >> 16) & 0xFFFF;
    idt[n].sel       = 0x08;
    idt[n].always0   = 0;
    idt[n].flags     = flags;
}

static void idt_set_gate(int n, uint32_t handler) {
    idt_set_gate_flags(n, handler, 0x8E);
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint32_t)isr0);
    }

    idt_set_gate(0,  (uint32_t)isr0);
    idt_set_gate(1,  (uint32_t)isr1);
    idt_set_gate(2,  (uint32_t)isr2);
    idt_set_gate(3,  (uint32_t)isr3);
    idt_set_gate(4,  (uint32_t)isr4);
    idt_set_gate(5,  (uint32_t)isr5);
    idt_set_gate(6,  (uint32_t)isr6);
    idt_set_gate(7,  (uint32_t)isr7);
    idt_set_gate(8,  (uint32_t)isr8);
    idt_set_gate(13, (uint32_t)isr13);
    idt_set_gate(14, (uint32_t)isr14);
    idt_set_gate(32, (uint32_t)isr32);
    idt_set_gate(33, (uint32_t)isr33);
    idt_set_gate(44, (uint32_t)isr44);
    idt_set_gate_flags(128, (uint32_t)isr128, 0xEE);   /* DPL=3 для syscall */

    __asm__ volatile ("lidt %0" : : "m"(idtp));
    __asm__ volatile ("sti");
}
