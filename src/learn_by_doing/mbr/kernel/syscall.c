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

char* getcwd(char* buf, uint32_t size) {
    return (char*)_syscall2(SYS_GETCWD, buf, size);
}

int32_t chdir(const char* path) {
    return _syscall1(SYS_CHDIR, path);
}

int32_t mkdir(const char* path) {
    return _syscall1(SYS_MKDIR, path);
}

int32_t stat(const char* path, struct stat* buf) {
    return _syscall2(SYS_STAT, path, buf);
}

struct dir* opendir(const char* name) {
    return (struct dir*)_syscall1(SYS_OPENDIR, name);
}

struct dir_entry* readdir(struct dir* dir) {
    return (struct dir_entry*)_syscall1(SYS_READDIR, dir);
}

int32_t rewinddir(struct dir* dir) {
    return _syscall1(SYS_REWINDDIR, dir);
}

int32_t closedir(struct dir* dir) {
    return _syscall1(SYS_CLOSEDIR, dir);
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
    register_syscall(SYS_GETCWD, sys_getcwd);
    register_syscall(SYS_CHDIR, sys_chdir);
    register_syscall(SYS_STAT, sys_stat);
    register_syscall(SYS_MKDIR, sys_mkdir);
    register_syscall(SYS_OPENDIR, sys_opendir);
    register_syscall(SYS_READDIR, sys_readdir);
    register_syscall(SYS_CLOSEDIR, sys_closedir);
    register_syscall(SYS_REWINDDIR, sys_rewinddir);
    printk("syscall init done\n");
}
