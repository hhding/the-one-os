#include "printk.h"
#include "interrupt.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "timer.h"
#include "keyboard.h"
#include "tss.h"
#include "process.h"

int test_var_a = 0, test_var_b = 0;

void k_thread_a(void * arg) {
    char* s = arg;
    while(1) {
        printk("%s %d ", s, test_var_a);
    }
}

void k_thread_b(void * arg) {
    char* s = arg;
    while(1) {
        printk("%s %d ", s, test_var_b);
    }
}

/* 测试用户进程 */
void u_prog_a(void) {
   while(1) {
      test_var_a++;
   }
}

/* 测试用户进程 */
void u_prog_b(void) {
   while(1) {
      test_var_b++;
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
    keyboard_init();
    tss_init();
    /*
    process_execute(u_prog_a, "user_prog_a");
    process_execute(u_prog_b, "user_prog_b");
    */
    thread_start("k_thread_a", 31, k_thread_a, "hello");
    thread_start("k_thread_b", 8, k_thread_b, "world");
    intr_enable();

	while(1) {
        printk("Main ");
    };
    return;
}
