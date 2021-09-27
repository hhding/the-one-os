;------------------ loader and kernel

LOADER_BASE_ADDR equ 0x8200
LOADER_START_SELECTOR equ 0x2

;------------------ gdt 描述符属性
; 总共段基址 32 位，段界限 20 位，段界限范围结合G位来确定最终界限
; 段基址 31-24，8比特
; G 23，0为1字节，1为4K字节
; D/B 22，有效地址和操作数大小。1表示32位环境，0为16位环境
; L 21，1表示64位环境
; AVL 20，无特别用途
; 段界限 19-16，4比特
; P 15 是否在内存中，设置为1
; DPL 14-13 2比特，4个特权级别
; S 12 0为系统段，1为非系统段
; TYPE 11-8 段的类型
; 段基址 7-0 8比特

; 段基址 31-16，16比特
; 段界限 15-0，16比特

DESC_G_4K   equ (1 << 23)
DESC_D_32   equ (1 << 22)
DESC_L      equ (0 << 21)
DESC_AVL    equ (0 << 20)
DESC_LIMIT_CODE2    equ (0xF << 16)
DESC_LIMIT_DATA2    equ DESC_LIMIT_CODE2
DESC_LIMIT_VIDEO2   equ (0x0 << 16)
DESC_P      equ (1 << 15)
DESC_DPL_0  equ (00b << 13)
DESC_DPL_1  equ (01b << 13)
DESC_DPL_2  equ (10b << 13)
DESC_DPL_3  equ (11b << 13)
DESC_S_CODE  equ (1 << 12)
DESC_S_DATA  equ DESC_S_CODE
DESC_S_SYS   equ (0 << 12)
DESC_TYPE_CODE  equ (1000b << 8)
DESC_TYPE_DATA  equ (0010b << 8)

DESC_CODE_HIGH4 equ (0x00 << 24) + \
    DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + \
    DESC_LIMIT_CODE2 + DESC_P + DESC_DPL_0 + DESC_S_CODE + DESC_TYPE_CODE + 0x00

DESC_DATA_HIGH4 equ (0x00 << 24) + \
    DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + \
    DESC_LIMIT_DATA2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x00

DESC_VIDEO_HIGH4 equ (0x00 << 24) + \
    DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + \
    DESC_LIMIT_VIDEO2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0b


;--------------- 选择子属性
RPL0    equ 00b
RPL1    equ 01b
RPL2    equ 10b
RPL3    equ 11b
TI_GDT  equ 000b
TI_LDT  equ 100b

; ------------ loader and kernel
PAGE_DIR_TABLE_POS equ 0x100000

; ----------- 页表相关属性
PG_P equ 1b
PG_RW_R equ 00b
PG_RW_W equ 10b
PG_US_S equ 000b
PG_US_U equ 100b

