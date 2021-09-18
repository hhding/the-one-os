org 0x8200

global _start

_start:
	mov si, hello_msg
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

hello_msg:
  	db 0xd, 0xa, 0xd, 0xa, "hello, I am kernel loaded from second sector", 0

