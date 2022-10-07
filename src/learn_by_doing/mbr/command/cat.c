#include "syscall.h"
#include "stdio.h"
#include "string.h"

void err_exit(const char* message) {
    printf(message);
    exit(-2);
}

int main(int argc, char** argv) {
    char buf[1024] = {0};
    int cnt = 0;
    if(argc != 2) {
        err_exit("cat: only 1 argument is supported\n");
    }
    if(argv[1][0] != '/') {
        err_exit("cat: only absolute path is supported\n");
    }
    int fd = open(argv[1], O_RDONLY);
    if(fd == -1) {
        err_exit("cat: open file failed\n");
    }
    while(1) {
        cnt = read(fd, buf, 1024);
        if(cnt == -1) {
            break;
        }
        write(1, buf, cnt);
    }
    return 0;
}
