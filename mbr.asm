org 0x7c00

jmp entry

entry:
    mov si, msg

putloop:
    mov al, [si]
    add si, 1
    cmp al, 0
    je fin

    mov ah, 0x0e
    mov bx, 15
    int 0x10
    jmp putloop

fin:
   hlt

msg:
  db "hello world", 0

times 510 - ($-$$) db 0
db 0x55, 0xaa

