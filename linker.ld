ENTRY(_start)

SECTIONS {
    . = 0x10000;

    .text : {
        *(.multiboot)
        *(.text)
        *(.rodata)
    }

    .data : { *(.data) }
    .bss  : { *(.bss) *(COMMON) }
}
