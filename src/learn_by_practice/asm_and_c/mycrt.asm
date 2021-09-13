global mystart
extern print

; LD 需要一个 entry，默认为 _start，这里在命令行里面指定为 mystart
mystart: 
	call print
; LD 需要一个 exit
exit:
	mov rax, 60
	xor rdi, rdi
	syscall

