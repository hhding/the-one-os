#ifndef __FS_FS_H
#define __FS_FS_H
#include "stdint.h"

#define MAX_FILES_PER_PART 4096     // 每个分区所支持最大创建的文件数
#define SECTOR_SIZE 512         // 扇区字节大小
#define BLOCK_SIZE 4096

#define MAX_PATH_LEN 512        // 路径最大长度
                                
enum file_types {
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
};

enum oflags {
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT = 4
};

enum whence {
    SEEK_SET = 1,
    SEEK_CUR,
    SEEK_END
};

struct stat {
    uint32_t st_no;
    uint32_t st_size;
    enum file_types st_filetype;
};

void filesystem_init(void);
int32_t sys_open(const char* pathname, uint8_t flag);
int32_t sys_close(int32_t fd);
int32_t sys_write(int32_t fd, const void* buf, uint32_t count);
int32_t sys_read(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_unlink(const char* pathname);
int32_t sys_mkdir(const char* pathname);
struct dir_entry* sys_opendir(const char* pathname);
void sys_rewinddir(struct dir* dir);
int32_t sys_rmdir(const char* pathname);
char* sys_getcwd(char* buf, uint32_t size);
int32_t sys_chdir(const char* path);
int32_t sys_stat(const char* path, struct stat* buf);
#endif

