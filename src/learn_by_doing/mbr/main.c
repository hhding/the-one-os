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
    char* buffer = get_kernel_pages(4);
    printk("buffer: 0x%x\n", (uint32_t)buffer);
	while(1) {};
}
