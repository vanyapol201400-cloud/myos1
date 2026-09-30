BITS 32

extern syscall_handler
global isr128

isr128:
    cli
    push 0                  ; err_code
    push 128                ; int_no
    jmp isr_common_80

extern isr_common
isr_common_80:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call syscall_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret
