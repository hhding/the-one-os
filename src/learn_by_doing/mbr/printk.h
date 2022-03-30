#include "stdint.h"

void update_cursor(uint16_t pos);
void printk(char* format, ...);
void console_init(void);
uint32_t syscall_write(char * s);
