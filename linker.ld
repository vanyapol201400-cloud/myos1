ENTRY(_start)

SECTIONS {
    . = 0x10000;

    .text : {
        *(.multiboot)
        *(.text)
        *(.rodata)
    }

    . = ALIGN(4096);
    .data : { *(.data) }

    . = ALIGN(4096);
    .bss  : { *(.bss) *(COMMON) }

    /DISCARD/ : { *(.eh_frame) *(.comment) *(.note*) }
}
