[bits 32]
section .text
global switch_to
; switch_to cur, next
; 调用 switch_to 的时候，当前的栈情况
; next      <- 6x4
; cur       <- 5x4
; ret addr  <- 4x4
; 调用 switch_to 栈情况
; esi       <- 3x4
; edi       <- 2x4
; ebx       <- 1x4
; ebp       <- 0 (esp)

switch_to:
    push esi
    push edi
    push ebx
    push ebp
    mov eax, [esp + 20] ; esp + 20 ==> cur
    mov [eax], esp      ; 把压栈后堆栈的 esp 地址存到 cur 第一个位置
    mov eax, [esp + 24] ; esp + 24 ==> next
    mov esp, [eax]      ; 跟上面的相反，把 next 上存的 esp 恢复过来，然后开始 pop 恢复寄存器
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret                 ; 最后，返回到 ret addr 继续执行，这里实际上返回到 schedule 函数
                        ; 整个函数只是恢复了 next 函数的运行环境，即 esp，ebp，ebx，edi和 esi

