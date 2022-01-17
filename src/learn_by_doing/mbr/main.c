#include "printk.h"
#include "interrupt.h"
#include "debug.h"
#include "memory.h"

void main()
{
    printk("\n\n");
    idt_init();
    mem_init();
    for(int i=0; i < 3; i++) {
        printk("%s %d, 0x%x\n", "Hello C Code OS!", 2020, 2020);
    }
    uint32_t* p = get_kernel_pages(2);
    printk("%x\n", p);
	while(1) {};
}
