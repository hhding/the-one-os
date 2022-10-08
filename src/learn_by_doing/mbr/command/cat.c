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

    printf("open %s for read\n", argv[1]);
    int fd = open(argv[1], O_RDONLY);
    if(fd == -1) {
        err_exit("cat: open file failed\n");
    }
    printf("open fd: %d\n", fd);
    while(1) {
        printf("3\n");
        cnt = read(fd, buf, 1024);
        if(cnt == -1) {
            break;
        }
        printf("4\n");
        write(1, buf, cnt);
    }
    printf("5\n");
    return 0;
}
