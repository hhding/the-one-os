#include "printk.h"
#include "interrupt.h"

void main()
{
    idt_init();
    for(int i=0; i < 3; i++) {
        printk("%s %d, 0x%x\n", "Hello C Code OS!", 2020, 2020);
    }
    asm volatile("sti");
	while(1) {};
}	
