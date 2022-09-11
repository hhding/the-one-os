#include "stdint.h"
#include "io.h"
#include "sync.h"
#include "stdio.h"
#include "string.h"
#include "fs.h"

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

static uint32_t _putchar(char c)
{
    char* p_strdst = (char*)(VGA_ADDR);
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
                
int stdout_write(char *s) {
    char c;
    int length = 0;
    lock_acquire(&print_lock);
    while ((c = *s++)) {
        _putchar(c);
        length ++;
    }
    lock_release(&print_lock);
    return length;
}

int printk(char* format, ...) {
    char buf[1024] = {0};
    va_list args;

    // arg 指向了 format
    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);
    return sys_write(0, buf, strlen(buf));
}

void console_init() {
    lock_init(&print_lock);
}

