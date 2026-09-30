ENTRY(_start)

SECTIONS {
    . = 0x10000;      /* ядро грузится с 0x10000 (64 КБ) */

    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
