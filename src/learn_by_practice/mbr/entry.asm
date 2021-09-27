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
;----------------- 创建页目录以及页表
; vaddr = 10bit PDE index + 10bit PTE index + 12bit offset(4096)
setup_page:
    mov ecx, 4096
    mov esi, 0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir

; 开始创建页目录项(PDE)
.create_pde ; 创建 Page Directory Entry
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x1000 ; 从这个地址开始，我们用来放页表（PTE）
    mov ebx, eax    ; 保存 eax，后面要把 eax 用于 PDE
    or eax, PG_US_U | PG_RW_W | PG_P    ; 属性位7，US=1, RW=1, P=1
    ; 两个都指向第一个页表
    mov [PAGE_DIR_TABLE_POS + 0x0], eax     ; 第一个4M，当前程序不受影响
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax   ; 768，3072==1024x3，3GB的高位

    sub eax, 0x1000                         ; 算出页目录位置
    mov [PAGE_DIR_TABLE_POS + 0x1000], eax  ; 最后一个页目录指向页表自身

; 创建页表项(PTE)，这个页表仅仅初始化了1MB内存，映射到低端1M内存
    mov ecx, 256   ; 分配1MB内存，每个PTE 4KB, PDE 在1MB上面
    mov esi, 0
    mov edx, PG_US_U | PG_RW_W | PG_P   ; 0-1MB 的物理内存地址
.create_pte:
    mov [ebx + esi*4], edx
    add edx, 4096
    inc esi
    loop .create_pte

; 创建内额其他页表的 PDE，内核其他页表的 PDE 都需要初始化
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000             ; 第二个页表
    or eax, PG_US_U | PG_RW_W | PG_P    ; 属性位7，US=1, RW=1, P=1
    mov ebx, PAGE_DIR_TABLE_POS
    mov ecx, 256
    mov esi, 769
    ; 页目录指向情况
    ; 0x0 --> 0 --> 初始化为1MB，最大4MB
    ; 0xc00 --> 0 --> 初始化为1MB，最大4MB
    ; 0xc01 --> 1
    ; 0xc02 --> 2
    ; ....
    ; 0x1000 --> 256
.create_kernel_pde:
    mov [ebx + esi*4], eax
    inc esi
    add eax, 0x1000
    loop .create_kernel_pde

    ret

p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    call setup_page
    sgdt [gdt_ptr]
    mov ebx, [gdt_ptr + 2]
    or dword [gdt_ptr + 2], 0xc0000000
    add esp, 0xc0000000
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    lgdt [gdt_ptr]

    mov ax, SELECTOR_VIDEO
    mov gs, ax
    mov byte [gs:0], 'P'

    jmp $

