#include "stdint.h"
#include "stdio.h"
#include "printk.h"
#include "string.h"
#include "syscall.h"

void itoa(uint32_t value, char** ptr2buf_ptr, int base) {
    uint32_t m = value % base;
    uint32_t v = value / base;

    // 函数返回时 ptr2buf_ptr 中 buf 的指针会被改变
    if(v) itoa(v, ptr2buf_ptr, base);
    // 取出 buffer 的地址
    // 完成后 buffer 的地址要加 1，移到后面一位
    // 当前 buffer 指向的字符填上数值
    *((*ptr2buf_ptr)++) = m < 10? m + '0': m - 10 + 'a';
}

int vsprintf(char* buf, const char* format, va_list arg) {
    char* buf_ptr = buf;
    char* tmp_ptr = NULL;
    char ch;

    while (( ch = *format++ )) {
        if ('%' == ch) {
            switch ((ch = *format++)) {
                case '%':
                    *buf_ptr++ = ch;
                    break;
                case 'c':
                    *buf_ptr++ = va_arg(arg, int);
                    break;
                case 's':
                    tmp_ptr = va_arg(arg, char *);
                    strcpy(buf_ptr, tmp_ptr);
                    buf_ptr += strlen(tmp_ptr);
                    break;
                case 'd':
                    itoa(va_arg(arg, int), &buf_ptr, 10);
                    break;
                case 'x':
                    itoa(va_arg(arg, int), &buf_ptr, 16);
                    break;
            }
        } else { *buf_ptr++ = ch; }
    }
    *buf_ptr = 0;

    return strlen(buf);
}

int sprintf(char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);
    return strlen(str);
}

int printf(const char* format, ...)
{
    char buf[1024] = {0};
    va_list args;

    // arg 指向了 format
    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);
    return write(buf);
}

