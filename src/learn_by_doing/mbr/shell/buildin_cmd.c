#include "buildin_cmd.h"
#include "syscall.h"
#include "stdio.h"
#include "shell.h"
#include "fs.h"
#include "string.h"

void normalize_path(char* old_path, char* new_path) {
    char name[MAX_FILE_NAME_LEN] = {0};
    char *sub_path = old_path;
    new_path[0] = '/';
    new_path[1] = 0;
    sub_path = path_parse(sub_path, name);

    while(name[0]) {
        if(!strcmp(name, ".")) goto normalize_out;
        if(!strcmp(name, "..")) {
            char* slash_ptr = strrchr(new_path, '/');
            // 对于到了根目录的情况，单独处理
            if(slash_ptr != new_path)
                *slash_ptr = 0;
            else
                *(slash_ptr+1) = 0;
            goto normalize_out;
        }
        // new_path == “/” 的情况单独处理
        if(new_path[1]) strcat(new_path, "/");
        strcat(new_path, name);
normalize_out:
        sub_path = path_parse(sub_path, name);
    }
}

int32_t make_abs_path(const char* path) {
    char abs_path[MAX_FILE_NAME_LEN] = {0};
    memset(abs_path, 0, MAX_FILE_NAME_LEN);
    if(path[0] == '/') {
        // 如果本来就已经是绝对路径了，直接把绝对路径赋值给 final_path 即可
        strcpy(final_path, path);
    } else {
        strcpy(abs_path, getcwd(final_path, MAX_PATH_LEN));
        strcat(abs_path, "/");
        strcat(abs_path, path);
        normalize_path(abs_path, final_path);
    }
    return 0;
}

void buildin_ls(uint32_t argc, char** argv) {
    char* target = NULL;
    struct dir_entry* dir_e = NULL;
    char ftype = '-';
    if(argc == 1) {
        target = getcwd(final_path, MAX_PATH_LEN);
    } else {
        make_abs_path(argv[1]);
        target = final_path;
    }

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
    make_abs_path(argv[1]);
    chdir(final_path);
    return final_path;
}

int32_t buildin_mkdir(uint32_t argc, char** argv) {
    make_abs_path(argv[1]);
    mkdir(final_path);
    return 0;
}

int32_t buildin_rmdir(uint32_t argc, char** argv) {
    make_abs_path(argv[1]);
    return 0;
}

int32_t buildin_rm(uint32_t argc, char** argv) {
    make_abs_path(argv[1]);
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
