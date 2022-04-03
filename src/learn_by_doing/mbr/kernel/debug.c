#include "debug.h"
#include "printk.h"
#include "interrupt.h"

void panic(char* filename, \
        int line, \
        const char* func, \
        const char* condition) {
    intr_disable();
    printk("\n\n\n!!!! error !!!!\nfilename: %s\nline: %d\nfunction: %s\ncondition: %s\n",
            filename, line, func, condition);
    while(1);
}
