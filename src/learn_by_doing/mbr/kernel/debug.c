#include "debug.h"
#include "printk.h"
#include "interrupt.h"

void panic(char* filename, \
        int line, \
        const char* func, \
        const char* condition) {
    intr_disable();
    char* buf[512] = {0};
    sprintf(buf, "\n\n\n!!!! error !!!!\nfilename: %s\nline: %d\nfunction: %s\ncondition: %s\n",
            filename, line, func, condition);
    stdout_write(buf);
    while(1);
}
