#include "stdint.h"

void update_cursor(uint16_t pos);
int printk(char* format, ...);
void console_init(void);
int stdout_write(char *s);
int32_t sys_putchar(char c);
