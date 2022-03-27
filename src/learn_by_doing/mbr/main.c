#include "printk.h"
#include "interrupt.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "timer.h"
#include "keyboard.h"
#include "tss.h"
#include "process.h"
#include "syscall.h"

int test_var_a = 0, test_var_b = 0;

void k_thread_a(void * arg) {
    char * p = malloc(63);
    printk("k_thread_a: 0x%x\n", (uint32_t)p);
    while(1);
}

void k_thread_b(void * arg) {
    while(1);
}

/* 测试用户进程 */
void __attribute__((optimize("O0"))) u_prog_a(void) {
   while(1) {
      test_var_a = getpid();
      //write("syscall write u_prog_a");
   }
}

/* 测试用户进程 */
void __attribute__((optimize("O0"))) u_prog_b(void) {
   while(1) {
      test_var_b = getpid();
      //write("syscall write u_prog_b");
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
    init_syscall();
    // Page Fault
    //*(char*)(0xb00000) = '1';
    process_execute(u_prog_a, "user_prog_a");
    process_execute(u_prog_b, "user_prog_b");
    thread_start("k_thread_a", 31, k_thread_a, "hello");
    thread_start("k_thread_b", 31, k_thread_b, "world");
    intr_enable();

	while(1) {
    //    printk("Main ");
    };
    return;
}
