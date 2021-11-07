[bits 32]

extern intr_handler21

global asm_intr21_entry

asm_intr21_entry:
    push ds
    push es
    pushad
    mov al,0x20                   ; 中断结束命令EOI
    out 0xa0,al                   ; 向从片发送
    out 0x20,al                   ; 向主片发送
    call intr_handler21
    jmp intr_exit

intr_exit:    
    popad
    pop es
    pop ds
    iretd

