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

void putchar(char c)
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
    return;
}
                

void puts(char *s) {
    char c;
    while(c = *s++) {
        putchar(c);
    }
}

void itoa(int value, char* buffer, int scale) {
    char * p = buffer;
    char * p_s;
    unsigned int tmp_int = value;
    char c;
    if(value < 0) {
        *p++ = '-';
        tmp_int = (unsigned long)(-(long)value);
    }
    p_s = p;

    do {
        *p++ = tmp_int % scale + '0';
        tmp_int /= scale;
    } while (tmp_int > 0);
    *p-- = '\0';

    do {
        c = *p;
        *p-- = *p_s;
        *p_s++ = c;
    } while(p > p_s);
}


int printf(char* fmt, ...)
{
    va_list arg;
    int int_temp;
    char ch;
    char char_temp;
    char buffer[512];
    char *string_temp;

    va_start(arg, fmt);
    while( ch = *fmt++ ) {
        if ('%' == ch) {
            switch (ch = *fmt++) {
                case '%':
                    putchar('%');
                    break;
                case 'c':
                    char_temp = va_arg(arg, int);
                    putchar(char_temp);
                    break;
                case 's':
                    string_temp = va_arg(arg, char *);
                    puts(string_temp);
                    break;
                case 'd':
                    int_temp = va_arg(arg, int);
                    itoa(int_temp, buffer, 10);
                    puts(buffer);
                    break;
                case 'x':
                    int_temp = va_arg(arg, int);
                    itoa(int_temp, buffer, 16);
                    puts(buffer);
                    break;
            }
        } else {
            putchar(ch);
        }
    }
    va_end(arg);
    return 0;
}
