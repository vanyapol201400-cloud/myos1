ENTRY(_start)
SECTIONS {
    . = 0x400000;
    .text : {
        *(.text._start)
        *(.text.startup)
        *(.text)
    }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
    /DISCARD/ : {
        *(.eh_frame) *(.eh_frame_hdr) *(.comment) *(.note*)
    }
}
