#include "common.h"

// LD 里面直接指定入口函数为 mymain
// 需要人肉处理 exit，否则会段错误
// 所有代码都不依赖 libc

void mymain()
{
	char msg[] = "hello world\n";
	my_write(1, msg, sizeof(msg));
	my_exit(0);
}
