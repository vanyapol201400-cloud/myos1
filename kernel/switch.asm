BITS 32

global task_switch_asm

; void task_switch_asm(uint32_t *old_esp, uint32_t new_esp);
; Сохраняет регистры на текущий стек, кладёт esp в *old_esp,
; загружает new_esp, восстанавливает регистры, ret -> в новую задачу.

task_switch_asm:
    push ebp
    push ebx
    push esi
    push edi
    pushfd

    mov eax, [esp + 24]      ; первый аргумент: old_esp (адрес)
    mov edx, [esp + 28]      ; второй аргумент: new_esp

    mov [eax], esp           ; *old_esp = текущий esp

    mov esp, edx             ; переключаемся на стек новой задачи

    popfd
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
