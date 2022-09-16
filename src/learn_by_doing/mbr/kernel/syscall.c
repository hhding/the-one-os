#include "syscall.h"
#include "thread.h"
#include "printk.h"
#include "interrupt.h"
#include "memory.h"
#include "fork.h"
#include "fs.h"

int32_t getpid(void) {
    return _syscall0(SYS_GETPID);
}

int32_t read(int32_t fd, char* str, uint32_t count) {
    return _syscall3(SYS_READ, fd, str, count);
}

int32_t write(int32_t fd, char* str, uint32_t count) {
    return _syscall3(SYS_WRITE, fd, str, count);
}

int32_t putchar(char c) {
    return _syscall1(SYS_PUTCHAR, c);
}

void* malloc(uint32_t size) {
    return (void*)_syscall1(SYS_MALLOC, size);
}

void free(void* ptr) {
    _syscall1(SYS_FREE, ptr);
}

int32_t fork(void) {
    return _syscall0(SYS_FORK);
}

void init_syscall() {
    printk("syscall init start\n");
    register_syscall(SYS_GETPID, sys_getpid);
    register_syscall(SYS_READ, sys_read);
    register_syscall(SYS_WRITE, sys_write);
    register_syscall(SYS_MALLOC, sys_malloc);
    register_syscall(SYS_FREE, sys_free);
    register_syscall(SYS_FORK, sys_fork);
    register_syscall(SYS_PUTCHAR, sys_putchar);
    printk("syscall init done\n");
}
