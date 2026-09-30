#include <stdint.h>

extern void print(const char *str);
extern void enter_user_mode(uint32_t entry, uint32_t user_stack);

static void user_hello(void) {
    /* syscall 1: putchar — печатает 'U','S','E','R' */
    __asm__ volatile ("int $0x80" : : "a"(1), "b"('U'));
    __asm__ volatile ("int $0x80" : : "a"(1), "b"('S'));
    __asm__ volatile ("int $0x80" : : "a"(1), "b"('E'));
    __asm__ volatile ("int $0x80" : : "a"(1), "b"('R'));
    __asm__ volatile ("int $0x80" : : "a"(1), "b"('\n'));

    /* syscall 0: exit */
    __asm__ volatile ("int $0x80" : : "a"(0));

    while (1) { }
}

static uint8_t user_stack[4096] __attribute__((aligned(16)));

void start_user_test(void) {
    print("\nEntering user mode...\n");
    uint32_t stack_top = (uint32_t)user_stack + sizeof(user_stack);
    stack_top &= ~0xF;
    enter_user_mode((uint32_t)user_hello, stack_top);
    print("Back\n");
}
