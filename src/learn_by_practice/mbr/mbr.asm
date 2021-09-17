org 0x7c00
jmp entry

entry:
	mov ax, 0x0820
	mov es, ax
	mov bx, 0	; buffer address es:bx

	mov ch, 0	; clyinder && 0xff
	mov dh, 0	; head
	mov cl, 2	; sector

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

	mov si, boot_msg

putloop:
    	mov al, [si]
    	add si, 1
    	cmp al, 0
    	je boot

    	mov ah, 0x0e
    	mov bx, 15
    	int 0x10
    	jmp putloop

boot:
	jmp 0x8200

fin:
   	hlt

error:
	mov si, err_msg
	jmp putloop

boot_msg:
  	db 0xa, "booting into second sector..", 0xa, 0

err_msg:
  	db 0xa, "error read disk", 0xa, 0

times 510 - ($-$$) db 0
db 0x55, 0xaa

