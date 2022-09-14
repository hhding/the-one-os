#include "buildin_cmd.h"
#include "syscall.h"
#include "stdio.h"

void buildin_ls(uint32_t argc, char** argv) {
    return;
}
char* buildin_cd(uint32_t argc, char** argv) {
    return "hello";
}
int32_t buildin_mkdir(uint32_t argc, char** argv) {
    return 0;
}
int32_t buildin_rmdir(uint32_t argc, char** argv) {
    return 0;
}
int32_t buildin_rm(uint32_t argc, char** argv) {
    return 0;
}
//void make_clear_abs_path(char* path, char* wash_buf);
void buildin_pwd(uint32_t argc, char** argv) {
    return;
}
void buildin_ps(uint32_t argc, char** argv) {
    return;
}
void buildin_clear(uint32_t argc, char** argv) {

}
void buildin_help(uint32_t argc, char** argv) {
    printf("You can use: ls, cd, mkdir, rmdir, rm, pwd, ps, clean and help.\n");
}