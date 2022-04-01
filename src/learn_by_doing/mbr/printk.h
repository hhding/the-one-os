#include "stdint.h"

void update_cursor(uint16_t pos);
int printk(char* format, ...);
void console_init(void);
int syscall_write(char * s);
