#include "vgastr.h"
#include "interrupt.h"

void main()
{
    idt_init();
    for(int i=0; i < 10; i++) {
        printf("Hello C Code OS!\n");
    }
	while(1) {};
}	
