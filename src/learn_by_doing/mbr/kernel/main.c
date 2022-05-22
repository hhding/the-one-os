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
#include "stdio.h"
#include "ide.h"
#include "fs.h"

int test_var_a = 0, test_var_b = 0;

void k_thread_a(void * arg) {
    int32_t fd;
    /*
    fd = sys_open("/file3", O_CREAT);
    if(fd > 0) sys_close(fd);
    sys_unlink("/file4");
    */
    // sys_mkdir("/hdir1");

    char* data = sys_malloc(4096);
    for(int i=0; i< 4096; i++) {
        data[i] = i % 58 + 'A';
    }
    fd = sys_open("/file3", O_RDWR);
    for(int i = 0; i < 6; i++) {
        sys_write(fd, data, 4096);
    }
    sys_close(fd);
    char buf[64] = {0};
    fd = sys_open("/file3", O_RDONLY);
    sys_read(fd, buf, 130);
    printk("got read buffer: %s\n", buf);

    if(fd < 0) {
        printk("open fail\n");
    } else {
        printk("close fd: %d\n", fd);
        sys_close(fd);
    }
    //sys_open("/file1", O_CREAT);
    char* p1 = malloc(63);
    char* p2 = malloc(12);
    printk("k_thread_a: 0x%x 0x%x\n", (uint32_t)p1, (uint32_t)p2);
    free(p2);
    char* p3 = malloc(60);
    printk("k_thread_a: 0x%x 0x%x\n", (uint32_t)p1, (uint32_t)p3);
    while(1) {
        thread_yield();
    }
}

void k_thread_b(void * arg) {
    char* name = "k_thread_b";
    //printk("%s, pid: %d\n", name, getpid());
    printk("name: %s, pid: %d\n", name, getpid());
    while(1) {
        thread_yield();
    }
}

/* 测试用户进程 */
void __attribute__((optimize("O0"))) u_prog_a(void) {
    char* p1 = malloc(63);
    char* p2 = malloc(12);
    free(p2);
    printf("u_prog_a: p1<0x%x>\n", (uint32_t)p1);
    while(1) {
        mtime_sleep(1000);
    }
}

/* 测试用户进程 */
void __attribute__((optimize("O0"))) u_prog_b(void) {
    test_var_b = getpid();
    printf("u_prog_b: pid<%d>\n", test_var_b);
    while(1);
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
    intr_enable();
    ide_init();
    filesystem_init();
    // Page Fault
    //*(char*)(0xb00000) = '1';
    //process_execute(u_prog_a, "user_prog_a");
    process_execute(u_prog_b, "user_prog_b");
    thread_start("k_thread_a", 31, k_thread_a, "hello");
    //thread_start("k_thread_b", 31, k_thread_b, "world");

    while(1) {
        mtime_sleep(1000);
    };
    return;
}
