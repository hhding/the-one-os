#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "stdint.h"
#include "fs.h"
#include "dir.h"

enum syscall_nr {
   SYS_GETPID,
   SYS_WRITE,
   SYS_MALLOC,
   SYS_FREE,
   SYS_FORK,
   SYS_READ,
   SYS_PUTCHAR,
   SYS_CLEAR,
   SYS_GETCWD,
   SYS_OPEN,
   SYS_CLOSE,
   SYS_LSEEK,
   SYS_UNLINK,
   SYS_MKDIR,
   SYS_OPENDIR,
   SYS_CLOSEDIR,
   SYS_CHDIR,
   SYS_RMDIR,
   SYS_READDIR,
   SYS_REWINDDIR,
   SYS_STAT,
   SYS_PS,
   SYS_EXECV,
   SYS_EXIT,
   SYS_WAIT,
   SYS_PIPE,
   SYS_FD_REDIRECT,
   SYS_HELP
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
char* getcwd(char* buf, uint32_t size);
int32_t chdir(const char* path);
int32_t stat(const char* path, struct stat* buf);
int32_t mkdir(const char* path);
int32_t stat(const char* path, struct stat* buf);
struct dir* opendir(const char* name);
struct dir_entry* readdir(struct dir* dir);
int32_t rewinddir(struct dir* dir);
int32_t closedir(struct dir* dir);
void ps(void);
int32_t rmdir(const char* path);
int32_t unlink(const char* path);
pid_t wait();
void exit(int32_t* status);
#endif

