#include "fs.h"
#include "string.h"
#include "file.h"
#include "syscall.h"
#include "stdio.h"

#define MAX_ARG_NR 16

static char cmd_line[MAX_PATH_LEN] = {0};
char cwd_cache[MAX_PATH_LEN] = {0};
char final_path[MAX_PATH_LEN] = {0};

void print_prompt(void) {
    printf("root@localhost:%s# ", cwd_cache);
}

static void readline(char* buf, int32_t count) {
    char* pos = buf;
    while(read(stdin_no, pos, 1) != -1 && (pos - buf) < count) {
        switch(*pos) {
            case '\n':
            case '\r':
                *pos = 0;
                putchar('\n');
                return;
            case '\b':
                if(pos > buf) {
                    pos[0] = 0;
                    --pos;
                    putchar('\b');
                }
                break;

            default:
                putchar(*pos);
                pos++;
        }
    }
    printf("readline: line too long\n");
}

void my_shell(void) {
    cwd_cache[0] = '/';
    while(1) {
        print_prompt();
        memset(final_path, 0, MAX_PATH_LEN);
        memset(cmd_line, 0, MAX_PATH_LEN);
        readline(cmd_line, MAX_PATH_LEN);
    }
}
