org 0x7c00
%include "boot/boot.inc"
jmp entry

entry:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov sp, 0x8000  ; 0x7c00 -> 0x8000
                    ; Total 1024 Byte, 512B for MBR and 512B for stack

    mov si, msg_hello
    call putloop

	mov ax, 0
	mov es, ax
	mov bx, LOADER_BASE_ADDR	; buffer address es:bx

	mov ch, 0	; clyinder && 0xff
	mov dh, 0	; head
	mov cl, 2	; sector
    call read_disk_loop

	mov si, boot_msg
    call putloop
	jmp LOADER_BASE_ADDR + 0x300

read_disk_loop:
	mov ah, 0x02	; read disk
	mov al, 1	; 1 sector
	mov dl, 0x80	; first disk
	int 0x13
	jc error
next:
	mov ax, es
	add ax, 0x0020
	mov es, ax
	add cl, 1	; next sector
	cmp cl, 8	; max 8 sector
	jbe read_disk_loop
    ret

putloop:
    mov al, [si]
    add si, 1
    cmp al, 0
    je put_ret

    mov ah, 0x0e
    mov bx, 15
    int 0x10
    jmp putloop
put_ret:
        ret

error:
	mov si, err_msg
	call putloop
    jmp fin

fin:
   	hlt

msg_hello:
  	db 0xd, 0xa, "Hello, this is a boot loader by dhh", 0

boot_msg:
  	db 0xd, 0xa, "MBR: Booting into second sector..", 0

err_msg:
  	db 0xa, "error read disk", 0xa, 0

times 510 - ($-$$) db 0
db 0x55, 0xaa

