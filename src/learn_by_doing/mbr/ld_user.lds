ENTRY(_start)
OUTPUT_ARCH(i386)
OUTPUT_FORMAT(elf32-i386)
SECTIONS
{
    /* 段起始位置 */

    . = 0x8049000;
    .text :
    {
        *(.text.startup)
        *(.text)
    }

    .data :
    {
        *(.data)
        *(.rodata)
    }

    .bss :
    {
        *(.bss)
    }

    .stab :
    {
        *(.stab)
    }
    
    .stabstr :
    {
        *(.stabstr)
    }

    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note.gnu.property) }
}
