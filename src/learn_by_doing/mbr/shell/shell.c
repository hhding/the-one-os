#include "fs.h"
#include "string.h"
#include "file.h"
#include "syscall.h"
#include "stdio.h"
#include "buildin_cmd.h"

#define MAX_ARG_NR 16

static char cmd_line[MAX_PATH_LEN] = {0};
char cwd_cache[MAX_PATH_LEN] = {0};
char final_path[MAX_PATH_LEN] = {0};

void print_prompt(void) {
    printf("root@toyos:%s# ", cwd_cache);
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

static int32_t cmd_parse(char* cmd_str, char** argv, char token) {
    char* next = cmd_str;
    int32_t argc = 0;
    while(*next) {
        // 找到第一个不是空格的字符
        while(*next == token) next++;
        // 字符末尾是空格的情况，直接结束
        if(*next == 0) break;
        argv[argc] = next;
        // 往下找，找到 0 或者空格为止
        while(*next && *next != token) next++;
        // 对于空格的情况，添上一个0，继续往下找。
        if(*next) *next++ = 0;
        if(argc > MAX_ARG_NR) return -1;
        argc++;
    }
    return argc;
}

void run_external(int argc, char** argv) {
    pid_t pid = fork();
    int32_t status;
    if(pid) {
        wait(&status);
    } else {
        make_abs_path(argv[0]);
        argv[0] = final_path;
        struct stat file_st;
        memset(&file_st, 0, sizeof(struct stat));
        if(stat(final_path, &file_st) == -1) {
            printf("shell: stat %s failed\n", final_path);
            exit(0);
        } else {
            printf("execv %s\n", argv[0]);
            execv(argv[0], argv);
        }
    }
}

static void cmd_execute(int32_t argc, char** argv) {
    char* cmd = argv[0];
    if(!strcmp("ls", cmd)) buildin_ls(argc, argv);
    else if(!strcmp("cd", cmd)) {
        char* path = buildin_cd(argc, argv);
        memset(cwd_cache, 0, MAX_FILE_NAME_LEN);
        strcpy(cwd_cache, path);
    }
    else if(!strcmp("pwd", cmd)) buildin_pwd(argc, argv);
    else if(!strcmp("ps", cmd)) buildin_ps(argc, argv);
    else if(!strcmp("mkdir", cmd)) buildin_mkdir(argc, argv);
    else if(!strcmp("rmdir", cmd)) buildin_rmdir(argc, argv);
    else if(!strcmp("rm", cmd)) buildin_rm(argc, argv);
    else if(!strcmp("help", cmd)) buildin_help(argc, argv);
    else {
        run_external(argc, argv);
    }

}

char* argv[MAX_ARG_NR] = {NULL};
int32_t argc = -1;

void my_shell(void) {
    cwd_cache[0] = '/';
    while(1) {
        print_prompt();
        memset(final_path, 0, MAX_PATH_LEN);
        memset(cmd_line, 0, MAX_PATH_LEN);
        readline(cmd_line, MAX_PATH_LEN);
        if(cmd_line[0] == 0) continue; // 空行
        argc = cmd_parse(cmd_line, argv, ' ');
        if(argc == -1) continue;
        cmd_execute(argc, argv);
    }
}
