#include "stdint.h"
#define NULL ((void*)0)
typedef char* va_list;
// 把 ap 指向第一个固定参数 v
#define va_start(ap, v) ap = (va_list)&v
// ap 指向下一个参数并返回其值
#define va_arg(ap, t) *((t*)(ap += 4))
#define va_end(ap) ap = NULL

int printf(const char *format, ...);
int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list ap);
int itoa(uint32_t value, char* buffer, int scale);
