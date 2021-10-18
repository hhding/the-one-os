#include <inttypes.h>
#include <sys/types.h>

ssize_t my_write(int fd, const void *buf, size_t size) {
    register int64_t rax __asm__ ("rax") = 1;
    register int rdi __asm__ ("rdi") = fd;
    register const void *rsi __asm__ ("rsi") = buf;
    register size_t rdx __asm__ ("rdx") = size;
    __asm__ __volatile__ (
        "syscall"
        : "+r" (rax)
        : "r" (rdi), "r" (rsi), "r" (rdx)
        : "rcx", "r11", "memory"
    );
    return rax;
}

void my_exit(int exit_status) {
    register int64_t rax __asm__ ("rax") = 60;
    register int rdi __asm__ ("rdi") = exit_status;
    __asm__ __volatile__ (
        "syscall"
        : "+r" (rax)
        : "r" (rdi)
        : "rcx", "r11", "memory"
    );
}

