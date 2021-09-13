global mystart
extern print

; 参考 https://zh.wikipedia.org/wiki/Crt0
;.text
;
;.globl _start
;
;_start: # _start is the entry point known to the linker
;    mov %rsp, %rbp    # setup a new stack frame
;    mov 0(%rbp), %rdi # get argc from the stack
;    lea 8(%rbp), %rsi # get argv from the stack
;    call main         # %rdi, %rsi are the first two args to main
;
;    mov %rax, %rdi    # mov the return of main to the first argument
;    call exit         # terminate the program
; LD 需要一个 entry，默认为 _start，这里在命令行里面指定为 mystart
mystart: 
	call print
; LD 需要一个 exit
exit:
	mov rax, 60
	xor rdi, rdi
	syscall

