#include "buildin_cmd.h"
#include "syscall.h"
#include "stdio.h"
#include "shell.h"
#include "fs.h"

void buildin_ls(uint32_t argc, char** argv) {
    char* target = NULL;
    struct dir_entry* dir_e = NULL;
    char ftype = '-';
    if(argc == 1) 
        target = getcwd(final_path, MAX_PATH_LEN);
    else
        target = argv[1];

    struct dir* dir = opendir(target);
    while((dir_e = readdir(dir))) {
        if(dir_e->f_type == FT_DIRECTORY) ftype = 'd';
        else if(dir_e->f_type == FT_REGULAR) ftype = '-';
        printf("%c %d %s\n", ftype, dir_e->i_no, dir_e->filename);
    }
    rewinddir(dir);
    closedir(dir);
    return;
}
char* buildin_cd(uint32_t argc, char** argv) {
    chdir(argv[1]);
    return argv[1];
}
int32_t buildin_mkdir(uint32_t argc, char** argv) {
    mkdir(argv[1]);
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
    if(argc != 1) {
        printf("pwd: too many arguments.\n");
    } else {
        if(getcwd(final_path, MAX_PATH_LEN)) {
            printf("%s\n", final_path);
        } else {
            printf("pwd: get current work directory failed.\n");
        }
    }
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