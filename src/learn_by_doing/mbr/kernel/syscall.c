#include "syscall.h"
#include "thread.h"
#include "printk.h"
#include "interrupt.h"
#include "memory.h"
#include "fork.h"
#include "fs.h"

int32_t getpid(void) {
    return _syscall0(SYS_getpid);
}

int32_t read(int32_t fd, char* str, uint32_t count) {
    return _syscall3(SYS_read, fd, str, count);
}

int32_t write(int32_t fd, char* str, uint32_t count) {
    return _syscall3(SYS_write, fd, str, count);
}

int32_t putchar(char c) {
    return _syscall1(SYS_putchar, c);
}

void* malloc(uint32_t size) {
    return (void*)_syscall1(SYS_malloc, size);
}

void free(void* ptr) {
    _syscall1(SYS_free, ptr);
}

int32_t fork(void) {
    return _syscall0(SYS_fork);
}

void init_syscall() {
    printk("syscall init start\n");
    register_syscall(SYS_getpid, sys_getpid);
    register_syscall(SYS_read, sys_read);
    register_syscall(SYS_write, sys_write);
    register_syscall(SYS_malloc, sys_malloc);
    register_syscall(SYS_free, sys_free);
    register_syscall(SYS_fork, sys_fork);
    register_syscall(SYS_putchar, sys_putchar);
    printk("syscall init done\n");
}
