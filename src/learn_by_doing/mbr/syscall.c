#include "syscall.h"
#include "thread.h"
#include "printk.h"
#include "interrupt.h"

uint32_t getpid(void) {
    return _syscall0(SYS_getpid);
}

uint32_t write(char* str) {
    return _syscall1(SYS_write, str);
}

void init_syscall() {
    register_syscall(SYS_getpid, syscall_getpid);
    register_syscall(SYS_write, puts);
}
