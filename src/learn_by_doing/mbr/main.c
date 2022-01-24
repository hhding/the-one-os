#include "printk.h"
#include "interrupt.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "timer.h"

void k_thread_a(void * arg) {
    char* s = arg;
    while(1) {
        printk("%s ", s);
    }
}

void k_thread_b(void * arg) {
    char* s = arg;
    while(1) {
        printk("%s ", s);
    }
}

void main()
{
    console_init();
    printk("\n\n");
    idt_init();
    mem_init();
    thread_init();
    timer_init();
    printk("starting thread 1...\n");
    thread_start("k_thread_a", 31, k_thread_a, "hello");
    printk("starting thread 2...\n");
    thread_start("k_thread_b", 8, k_thread_b, "world");
    printk("enable interrupt...\n");
    intr_enable();
	while(1) {
        printk("Main ");
    };
    return;
}
