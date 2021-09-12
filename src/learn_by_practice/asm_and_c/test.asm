global mystart
extern print

mystart: 
	call print
	mov rax, 60
	xor rdi, rdi
	syscall

