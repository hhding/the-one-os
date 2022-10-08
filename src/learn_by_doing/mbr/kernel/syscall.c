#include "syscall.h"

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

void ps(void) {
    _syscall0(SYS_PS);
}

int32_t rmdir(const char* path) {
    return _syscall1(SYS_RMDIR, path);
}

int32_t unlink(const char* path) {
    return _syscall1(SYS_UNLINK, path);
}

pid_t wait(int32_t* status) {
    return _syscall1(SYS_WAIT, status);
}

void exit(int32_t status) {
    _syscall1(SYS_EXIT, status);
}

int32_t execv(const char* path, char* argv[]) {
    return _syscall2(SYS_EXECV, path, argv);
}

int32_t open(const char* path, int flags) {
    return _syscall2(SYS_OPEN, path, flags);
}

int32_t close(uint32_t fd) {
    return _syscall1(SYS_CLOSE, fd);
}

