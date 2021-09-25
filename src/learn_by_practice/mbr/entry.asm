%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
    jmp loader_start

    GDT_BASE:           dd  0x00000000
                        dd  0x00000000
    CODE_DESC:          dd  0x0000FFFF
                        dd  DESC_CODE_HIGH4
    DATA_STACK_DESC:    dd  0x0000FFFF
                        dd  DESC_DATA_HIGH4
    VIDEO_DESC:         dd  0x80000008
                        dd  DESC_VIDEO_HIGH4

    GDT_SIZE    equ $ - GDT_BASE
    GDT_LIMIT   equ GDT_SIZE - 1
    SELECTOR_CODE   equ  (0x0001 << 3) + TI_GDT + RPL0
    SELECTOR_DATA   equ  (0x0002 << 3) + TI_GDT + RPL0
    SELECTOR_VIDEO  equ  (0x0003 << 3) + TI_GDT + RPL0

    gdt_ptr:    dw  GDT_LIMIT
                dd  GDT_BASE

    
    loader_start:
; ------------ 准备进入保护模式 ------------------
; 1 打开 A20
; 2 加载 gdt
; 3 将 cr0 的 pe 位置 1

    ; ------------ 打开 A20 ------------------
    in al, 0x92
    or al, 0000_0010B
    out 0x92, al

    ; ------------- 加载 gdt ------------------
    lgdt [gdt_ptr]

    ; ------------- cr0 第 0 位置 1 -----------
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax
    
    ; 刷新流水线，进入 32 位代码
    jmp SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x8200

    hlt


