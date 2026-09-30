ENTRY(_start)

SECTIONS {
    . = 1M;                     /* GRUB грузит Multiboot-ядра в 1 МБ */

    .multiboot : { *(.multiboot) }
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
