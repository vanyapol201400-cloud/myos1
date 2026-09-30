BITS 32

global enter_user_mode

; void enter_user_mode(uint32_t entry, uint32_t user_stack);

enter_user_mode:
    cli
    mov eax, [esp + 4]
    mov edx, [esp + 8]

    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    push 0x23
    push edx
    pushfd
    pop ecx
    or ecx, 0x200
    push ecx
    push 0x1B
    push eax
    iret
