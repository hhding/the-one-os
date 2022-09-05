#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "stdint.h"
#define SYS_getpid  0
#define SYS_write   1
#define SYS_malloc  2
#define SYS_free    3
#define SYS_fork    4
#define _syscall0(name) ({ int _res; asm volatile("int $0x80" : "=a"(_res) : "a"(name) : "memory"); _res; })
#define _syscall1(name, arg1) ({ int _res; asm volatile("int $0x80" : "=a"(_res) : "a"(name), "b"(arg1) : "memory"); _res; })
#define _syscall2(name, arg1, arg2) ({ int _res; asm volatile("int $0x80" : "=a"(_res) : "a"(name), "b"(arg1) : "memory", "c"(arg2) : "memory"); _res; })

void init_syscall(void);
uint32_t getpid(void);
uint32_t fork(void);
void* malloc(uint32_t size);
void free(void* ptr);
uint32_t write(uint32_t fd, char* str);
#endif

