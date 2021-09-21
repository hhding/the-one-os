;------------------ loader and kernel

LOADER_BASE_ADDR equ 0x8200
LOADER_START_SELECTOR equ 0x2

;------------------ gdt 描述符属性
DESC_G_4K   equ 1 << 24
DESC_D_32   equ  1_
DESC_L      equ   0_
DESC_AVL    equ     0_
DESC_LIMIT_CODE2    equ 0_
DESC_LIMIT_DATA2    equ DESC_LIMIT_CODE2
DESC_LIMIT_VIDEO2   equ 0000_
DESC_P      equ 1_
DESC_DPL_0  equ
DESC_DPL_1  equ
DESC_DPL_2  equ
DESC_DPL_3  equ
DESC_S_CODE  equ
DESC_S_DATA  equ
DESC_S_sys  equ
DESC_TYPE_CODE  equ
DESC_TYPE_DATA  equ
DESC_CODE_HIGH4 equ
DESC_DATA_HIGH4 equ
DESC_VIDEO_HIGH4 equ

;--------------- 选择子属性
RPL0    equ 00b
RPL1    equ 01b
RPL2    equ 10b
RPL3    equ 11b
TI_GDT  equ 000b
TI_LDT  equ 100b
