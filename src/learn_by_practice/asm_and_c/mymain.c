#include "common.h"

void mymain()
{
	char msg[] = "hello world\n";
	my_write(1, msg, sizeof(msg));
	my_exit(0);
}
