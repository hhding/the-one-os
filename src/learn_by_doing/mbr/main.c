#include "printk.h"
#include "interrupt.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"

void k_thread_a(void * arg) {
    char* s = arg;
    while(1) {
        printk("%s ", s);
    }
}

void main()
{
    printk("\n\n");
    idt_init();
    mem_init();
    for(int i=0; i < 3; i++) {
        printk("%s %d, 0x%x\n", "Hello C Code OS!", 2020, 2020);
    }

    thread_start("k_thread_a", 31, k_thread_a, "hello");
	while(1) {};
}
