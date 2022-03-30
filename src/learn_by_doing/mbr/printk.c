#include "stdint.h"
#include "io.h"
#include "sync.h"
#include "stdio.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define CRT_CTRL_ADDR_R 0x3D4
#define CRT_CTRL_DATA_R 0x3D5

#define VGA_ADDR 0xc00b8000

static struct lock print_lock;

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
    char* p_strdst = (char*)(VGA_ADDR);
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

uint32_t _putchar(char c)
{
    char* p_strdst = (char*)(VGA_ADDR);
    lock_acquire(&print_lock);
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
    lock_release(&print_lock);
    return 1;
}
                
uint32_t syscall_write(char *s) {
    char c;
    int length = 0;
    while ((c = *s++)) {
        _putchar(c);
        length ++;
    }
    return length;
}

uint32_t printk(char* fmt, ...) {
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
                    length += _putchar('%');
                    break;
                case 'c':
                    char_temp = va_arg(arg, int);
                    length += _putchar(char_temp);
                    break;
                case 's':
                    string_temp = va_arg(arg, char *);
                    length += syscall_write(string_temp);
                    break;
                case 'd':
                    int_temp = va_arg(arg, int);
                    itoa(int_temp, buffer, 10);
                    length += syscall_write(buffer);
                    break;
                case 'x':
                    int_temp = va_arg(arg, int);
                    itoa(int_temp, buffer, 16);
                    length += syscall_write(buffer);
                    break;
            }
        } else { length += _putchar(ch); }
    }

    va_end(arg);
    return length;
}

void console_init() {
    lock_init(&print_lock);
}
