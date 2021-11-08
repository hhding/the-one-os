#include "vgastr.h"
#include "interrupt.h"

void main()
{
    idt_init();
    for(int i=0; i < 10; i++) {
        printf("Hello C Code OS! %d\n", 2020);
    }
    asm volatile("sti");
	while(1) {};
}	
