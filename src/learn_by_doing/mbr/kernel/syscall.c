#include "syscall.h"
#include "thread.h"
#include "printk.h"
#include "interrupt.h"
#include "memory.h"
#include "fork.h"
#include "fs.h"

uint32_t getpid(void) {
    return _syscall0(SYS_getpid);
}

uint32_t write(uint32_t fd, char* str, uint32_t count) {
    return _syscall3(SYS_write, fd, str, count);
}

void* malloc(uint32_t size) {
    return (void*)_syscall1(SYS_malloc, size);
}

void free(void* ptr) {
    _syscall1(SYS_free, ptr);
}

uint32_t fork(void) {
    return _syscall0(SYS_fork);
}

void init_syscall() {
    printk("syscall init start\n");
    register_syscall(SYS_getpid, sys_getpid);
    register_syscall(SYS_write, sys_write);
    register_syscall(SYS_malloc, sys_malloc);
    register_syscall(SYS_free, sys_free);
    register_syscall(SYS_fork, sys_fork);
    printk("syscall init done\n");
}
