#include "syscall.h"
#include "thread.h"
#include "printk.h"
#include "interrupt.h"
#include "memory.h"

uint32_t getpid(void) {
    return _syscall0(SYS_getpid);
}

uint32_t write(char* str) {
    return _syscall1(SYS_write, str);
}

void* malloc(uint32_t size) {
    return (void*)_syscall1(SYS_malloc, size);
}

void free(void* ptr) {
    _syscall1(SYS_free, ptr);
}

void init_syscall() {
    printk("syscall init start\n");
    register_syscall(SYS_getpid, sys_getpid);
    register_syscall(SYS_write, stdout_write);
    register_syscall(SYS_malloc, sys_malloc);
    register_syscall(SYS_free, sys_free);
    printk("syscall init done\n");
}
