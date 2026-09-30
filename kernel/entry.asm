BITS 32
global _start
extern kernel_main

section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00000000
    dd -(0x1BADB002 + 0x00000000)

section .text
_start:
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang
