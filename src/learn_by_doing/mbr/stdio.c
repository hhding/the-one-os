#include "stdint.h"
#include "stdio.h"
#include "printk.h"
#include "string.h"
#include "syscall.h"

uint32_t puts(char *s) {
    return syscall_write(s);
}

int itoa(uint32_t value, char* buffer, int scale) {
    char * p = buffer;
    char * p_s;
    uint32_t tmp_int = value;
    char c;
    uint32_t i2c;

    /*
    if(value < 0) {
        *p++ = '-';
        tmp_int = (unsigned long)(-(long)value);
    }
    */
    p_s = p;

    do {
        i2c = tmp_int % scale; 
        *p++ = i2c >=10? i2c - 10 + 'a': i2c + '0';
        tmp_int /= scale;
    } while (tmp_int > 0);
    *p-- = '\0';

    do {
        c = *p;
        *p-- = *p_s;
        *p_s++ = c;
    } while(p > p_s);
    return p - buffer;
}

int vsprintf(char* buf, const char* format, va_list arg) {
    char* buf_ptr = buf;
    char* tmp_ptr = NULL;
    uint32_t int_tmp;
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
                    strcpy(buf, va_arg(arg, char *));
                    buf_ptr += strlen(tmp_ptr);
                    break;
                case 'd':
                    int_tmp = va_arg(arg, int);
                    int_tmp = itoa(int_tmp, buf_ptr, 10);
                    buf_ptr += int_tmp;
                    break;
                case 'x':
                    int_tmp = va_arg(arg, int);
                    int_tmp = itoa(int_tmp, buf_ptr, 16);
                    buf_ptr += int_tmp;
                    break;
            }
        } else { *buf_ptr++ = ch; }
    }
    *buf_ptr = 0;

    return strlen(buf);
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

