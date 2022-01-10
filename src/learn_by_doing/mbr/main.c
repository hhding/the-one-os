#include "printk.h"
#include "interrupt.h"
#include "debug.h"

void main()
{
    idt_init();
    for(int i=0; i < 3; i++) {
        printk("%s %d, 0x%x\n", "Hello C Code OS!", 2020, 2020);
    }
    asm volatile("sti");
    ASSERT(1==2);
	while(1) {};
}
