BITS 32
extern isr_handler

%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push 0
    push %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push %1
    jmp isr_common
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 128
ISR_NOERR 44

isr_common:
    pusha                   ; eax, ecx, edx, ebx, esp, ebp, esi, edi (32-бит)

    ; Сохраняем ds, es, fs, gs как 32-битные значения
    xor eax, eax
    mov ax, ds
    push eax
    xor eax, eax
    mov ax, es
    push eax
    xor eax, eax
    mov ax, fs
    push eax
    xor eax, eax
    mov ax, gs
    push eax

    ; Загружаем kernel data
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    ; Восстанавливаем gs, fs, es, ds
    pop eax
    mov gs, ax
    pop eax
    mov fs, ax
    pop eax
    mov es, ax
    pop eax
    mov ds, ax

    popa
    add esp, 8              ; int_no + err_code
    iret
