#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "stdint.h"
enum syscall_nr {
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK,
    SYS_PUTCHAR,
    SYS_READ,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_LSEEK,
    SYS_UNLINK,
    SYS_MKDIR,
    SYS_RMDIR,
    SYS_CHDIR,
    SYS_OPENDIR,
    SYS_CLOSEDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_STAT,
    SYS_PS,
};

#define _syscall0(name) ({ int _res; asm volatile("int $0x80" : "=a"(_res) : "a"(name) : "memory"); _res; })
#define _syscall1(name, arg1) ({ int _res; asm volatile("int $0x80" : "=a"(_res) : "a"(name), "b"(arg1) : "memory"); _res; })
#define _syscall2(name, arg1, arg2) ({ int _res; asm volatile("int $0x80" : "=a"(_res) : "a"(name), "b"(arg1), "c"(arg2) : "memory"); _res; })
#define _syscall3(name, arg1, arg2, arg3) ({ int _res; asm volatile("int $0x80" : "=a"(_res) : "a"(name), "b"(arg1), "c"(arg2), "d"(arg3)  : "memory"); _res; })

void init_syscall(void);
int32_t getpid(void);
int32_t fork(void);
void* malloc(uint32_t size);
void free(void* ptr);
int32_t write(int32_t fd, char* str, uint32_t count);
int32_t read(int32_t fd, char* str, uint32_t count);
int32_t putchar(char c);
#endif

