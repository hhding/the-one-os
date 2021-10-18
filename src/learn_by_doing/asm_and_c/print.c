#include "common.h"

void print()
{
	char msg[] = "hello world\n";
	my_write(1, msg, sizeof(msg));
}
