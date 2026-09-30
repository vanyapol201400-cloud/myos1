; boot.asm — boot sector (512 байт). Грузится BIOS в 0x7C00.
BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Сохранить номер загрузочного диска
    mov [boot_drive], dl

    ; Вывести сообщение
    mov si, msg_boot
    call print

    ; Загрузить ядро: секторы 2..MAX (в память 0x1000:0000 = 0x10000)
    ; Ядро грузится по физическому адресу 0x10000
    mov ax, 0x1000
    mov es, ax
    xor bx, bx              ; ES:BX = 0x1000:0000

    mov ah, 0x02            ; функция чтения
    mov al, 127             ; 127 секторов = 65024 байт
    mov ch, 0               ; цилиндр 0
    mov cl, 2               ; с сектора 2
    mov dh, 0               ; головка 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    mov si, msg_loaded
    call print

    ; Включить A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Загрузить GDT
    lgdt [gdt_desc]

    ; Перейти в protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:pm_start

disk_error:
    mov si, msg_err
    call print
    jmp $

print:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp print
.done:
    ret

boot_drive: db 0
msg_boot:   db "Booting myos...", 13, 10, 0
msg_loaded: db "Kernel loaded.", 13, 10, 0
msg_err:    db "Disk error!", 0

; GDT
gdt_start:
    dq 0x0000000000000000
gdt_code:
    dq 0x00CF9A000000FFFF
gdt_data:
    dq 0x00CF92000000FFFF
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

BITS 32
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Прыжок в ядро (0x10000)
    jmp 0x10000

times 510-($-$$) db 0
dw 0xAA55
