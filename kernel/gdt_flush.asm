BITS 32

global gdt_flush
global tss_flush

; void gdt_flush(uint32_t gp_ptr);
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10       ; kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.reload   ; kernel code segment
.reload:
    ret

; void tss_flush(void);
tss_flush:
    mov ax, 0x28       ; TSS segment = 5 * 8 = 40 = 0x28
    ltr ax
    ret
