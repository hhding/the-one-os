#include "stdint.h"
#include "io.h"
#include <stdarg.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define CRT_CTRL_ADDR_R 0x3D4
#define CRT_CTRL_DATA_R 0x3D5

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(CRT_CTRL_ADDR_R, 0x0A);
	outb(CRT_CTRL_DATA_R, (inb(CRT_CTRL_DATA_R) & 0xC0) | cursor_start);

	outb(CRT_CTRL_ADDR_R, 0x0B);
	outb(CRT_CTRL_DATA_R, (inb(CRT_CTRL_DATA_R) & 0xE0) | cursor_end);
}

void disable_cursor()
{
	outb(CRT_CTRL_ADDR_R, 0x0A);
	outb(CRT_CTRL_DATA_R, 0x20);
}

uint16_t get_cursor_position(void)
{
    uint16_t pos = 0;
    outb(CRT_CTRL_ADDR_R, 0x0F);
    pos |= inb(CRT_CTRL_DATA_R);
    outb(CRT_CTRL_ADDR_R, 0x0E);
    pos |= ((uint16_t)inb(CRT_CTRL_DATA_R)) << 8;
    return pos;
}

void update_cursor(uint16_t pos)
{
    // 处理滚屏的问题
    char* p_strdst = (char*)(0xb8000);
    if(pos >= 2000) {
        for(int i = 0; i < pos; i++) {
            *(p_strdst + i * 2) = *(p_strdst + i *2 + 80*2);
            *(p_strdst + i * 2 + 1) = *(p_strdst + i *2 +1 + 80*2);
        }
        for(int i = pos + 1; i < 2000; i++) {
            *(p_strdst + i * 2) = 0x20;
            *(p_strdst + i * 2 + 1) = 0x07;
        }
        pos = pos - 80;
    }

	outb(CRT_CTRL_ADDR_R, 0x0F);
	outb(CRT_CTRL_DATA_R, (uint8_t) (pos & 0xFF));
	outb(CRT_CTRL_ADDR_R, 0x0E);
	outb(CRT_CTRL_DATA_R, (uint8_t) ((pos >> 8) & 0xFF));
}

uint32_t putchar(char c)
{
    char* p_strdst = (char*)(0xb8000);
    // CR(\r): 0x0d, LF(\n): 0xa, BS: 0x8
    uint16_t pos = get_cursor_position();

    switch(c) {
        case 0x0d:
            pos = pos - pos % VGA_WIDTH;
            update_cursor(pos);
            break;
        case 0x0a:
            pos = pos - pos % VGA_WIDTH + VGA_WIDTH;
            update_cursor(pos);
            break;
        case 0x08:
            *(p_strdst + pos * 2) = 0x20;
            *(p_strdst + pos * 2 + 1) = 0x07;
            if(pos >= 1)
                pos = pos - 1;
            update_cursor(pos);
            break;
        default:
            *(p_strdst + pos * 2) = c;
            update_cursor(pos + 1);
    }
    return 1;
}
                

uint32_t puts(char *s) {
    char c;
    int length = 0;
    while ((c = *s++)) {
        putchar(c);
        length ++;
    }
    return length;
}

void itoa(uint32_t value, char* buffer, int scale) {
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
}


uint32_t printk(char* fmt, ...)
{
    uint32_t int_temp;
    char ch;
    char char_temp;
    char buffer[512];
    char *string_temp;
    va_list arg;
    uint32_t length = 0;

    va_start(arg, fmt);

    while (( ch = *fmt++ )) {
        if ('%' == ch) {
            switch ((ch = *fmt++)) {
                case '%':
                    length += putchar('%');
                    break;
                case 'c':
                    char_temp = va_arg(arg, int);
                    length += putchar(char_temp);
                    break;
                case 's':
                    string_temp = va_arg(arg, char *);
                    length += puts(string_temp);
                    break;
                case 'd':
                    int_temp = va_arg(arg, int);
                    itoa(int_temp, buffer, 10);
                    length += puts(buffer);
                    break;
                case 'x':
                    int_temp = va_arg(arg, int);
                    itoa(int_temp, buffer, 16);
                    length += puts(buffer);
                    break;
            }
        } else { length += putchar(ch); }
    }

    va_end(arg);
    return length;
}
