%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
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

    times 0x300 -  ($-$$) db 0
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
    ; 4K Bytes => 1 Page
    mov ecx, 4096
    mov esi, 0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir

; 开始创建页目录项(PDE)
.create_pde: ; 创建 Page Directory Entry
    ; PAGE_DIR_TABLE_POS equ 0x100000 1M 开始
    mov eax, PAGE_DIR_TABLE_POS
    ; 1M + 4K => 0x101000 
    add eax, 0x1000 ; 4K 页目录，紧接这个页目录，我们用来放页表（PTE）
    mov ebx, eax    ; 保存 eax，后面要把 eax 用于 PDE
    or eax, PG_US_U | PG_RW_W | PG_P    ; 属性位7，US=1, RW=1, P=1
    ; 两个都指向第一个页表
    mov [PAGE_DIR_TABLE_POS + 0x0], eax     ; 第一个4M，当前程序不受影响
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax   ; 768，0xc00=3072==1024x3，3GB的高位

    sub eax, 0x1000                         ; 算出页目录位置
    mov [PAGE_DIR_TABLE_POS + 0x1000 - 4], eax  ; 最后一个页目录指向页表自身

; 创建页表项(PTE)，这个页表仅仅初始化了1MB内存，映射到低端1M内存
    mov ecx, 256   ; 分配1MB内存，每个PTE 4KB, PDE 在1MB上面
    mov esi, 0
    mov edx, PG_US_U | PG_RW_W | PG_P   ; 0-1MB 的物理内存地址
.create_pte:
    ; 4个字节递增，每递增4个字节就增长一个 PTE
    ; edx 这里是从 0 开始，0，4k，8k，总共256个，也就是覆盖了 1MB
    ; 查看方式 xp 0x101000
    mov [ebx + esi*4], edx
    add edx, 4096
    inc esi
    loop .create_pte

; 创建内核其他页表的 PDE，内核其他页表的 PDE 都需要初始化
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000             ; 第二个页表
    or eax, PG_US_U | PG_RW_W | PG_P    ; 属性位7，US=1, RW=1, P=1
    mov ebx, PAGE_DIR_TABLE_POS
    mov ecx, 254    ; 范围为第769~1022的所有目录项数量
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

   ;将gdt的基址加上0xc0000000使其成为内核所在的高地址
    mov ebx, [gdt_ptr + 2]
    ;视频段是第3个段描述符,每个描述符是8字节,故0x18。
    or dword [ebx + 0x18 + 4], 0xc0000000      
    ;段描述符的高4字节的最高位是段基址的31~24位

    add dword [gdt_ptr + 2], 0xc0000000
    add esp, 0xc0000000 ; 将栈指针同样映射到内核地址

   ; 把页目录地址赋给cr3
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

   ; 打开cr0的pg位(第31位)
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    lgdt [gdt_ptr]

    mov eax, KERNEL_START_SECTOR
    mov ebx, KERNEL_BIN_BASE_ADDR
    mov ecx, 200
    call rd_disk_m_32
    
    jmp SELECTOR_CODE:enter_kernel

enter_kernel:
    call kernel_init
    mov esp, 0xc009f000
    jmp KERNEL_ENTRY_POINT


    ; 先不区分数据和video选择子，64位后主要还是靠页表区分
    mov ax, SELECTOR_DATA
    mov gs, ax
    mov byte [gs:0], 'P'

    jmp $


; ------------ 解析和重新加载在内存中的 ELF 文件 ----------------
kernel_init:
    xor eax, eax
    xor ebx, ebx    ; ebx 记录程序头表地址
    xor ecx, ecx    ; cx 记录程序头表中 Program header 数量
    xor edx, edx    ; dx 记录 Program header 大小，即 e_phentsize
    mov dx, [KERNEL_BIN_BASE_ADDR + 0x2a]
    mov ebx, [KERNEL_BIN_BASE_ADDR + 0x1c]  ; 程序头的偏移
    add ebx, KERNEL_BIN_BASE_ADDR           ; 这个才是程序头表地址
    mov cx, [KERNEL_BIN_BASE_ADDR + 0x2c]
    
.each_segment:
    cmp dword [ebx + 0], 0x1
    jne .next_segment
    ; memcpy(dst, src, size) <-  从右向左压栈
    push dword [ebx + 16]   ; p_filesz, size
    mov eax, [ebx + 4]      ; p_offset
    add eax, KERNEL_BIN_BASE_ADDR   ; src
    push eax
    push dword [ebx + 8]    ; p_vaddr, dst
    
    call mem_cpy
    add esp, 12

.next_segment:
    add ebx, edx
    loop .each_segment
    ret

; --------------------------------------------------------
; 输入：栈中三个参数（dst，src，size）
; 输出：无
; --------------------------------------------------------
mem_cpy:
    cld ; 给 movsb 用，会自动修改 esi，edi
    push ebp
    mov ebp, esp
    push ecx    ; ecx 在前面有用，先备份，给 movsb 用

    mov edi, [ebp + 8]  ; dst
    mov esi, [ebp + 12] ; src
    mov ecx, [ebp + 16] ; size
    rep movsb

    pop ecx
    pop ebp
    ret

; --------------------------------------------------------
; 功能：读取硬盘 n 个扇区
rd_disk_m_32:
; eax = lba 扇区号
; ebx = 将数据写入的内存地址
; ecx = 读入的扇区数
; --------------------------------------------------------
    mov esi, eax    ; backup eax
    mov di, cx      ; backup cx

; 读写硬盘
; 第一步：设置要读取的扇区数
    mov dx, 0x1f2
    mov al, cl
    out dx, al  ; 读取的扇区数

    mov eax, esi    ; 恢复 ax

; 第二步：将 lba 地址存入 0x1f3 - 0x1f6
    ; lba 地址7-0位写入端口 0x1f3
    mov dx, 0x1f3
    out dx, al

    ; lba 地址15-8位写入端口 0x1f4
    mov dx, 0x1f4
    mov cl, 8   ; cl 前面用完了，这里可以直接用
    shr eax, cl ; 右移8位，中间8位变成低8位
    out dx, al

    ; lba 地址23-16位写入端口 0x1f5
    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    shr eax, cl
    and al, 0x0f    ; 这里只需要低4位, lba 地址 24-27 位
    or al, 0xe0     ; 7-4位设置为 1110，表示 lba 模式
    mov dx, 0x1f6
    out dx, al

; 第三步：向 0x1f7 端口写入读命令 0x20
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

; 第四步：检测硬盘专题

.not_ready:
    ; 同一个端口 0x1f7
    nop
    in al, dx
    and al, 0x88    ; 第三位为1表示硬盘控制器已经准备好数据传输
                    ; 第七位位1表示硬盘忙
    cmp al, 0x08
    jnz .not_ready

; 第五步：从 0x1f0 端口读取数据
; ECX -> DI 是前面传进来要读多少扇区
    mov ax, di
    mov dx, 256
    mul dx ; 每个扇区512字节，每次读取一个字（2个字节），所以只要 256 就可以了。
            ; 计算结果存在 EDX:EAX 上，EAX 应该为0xc800，后面应该是 ECX 的值

    mov dx, 0x1f0
    mov cx, ax
.go_on_read:
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop .go_on_read
    ret

