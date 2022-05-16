#include "fs.h"
#include "super_block.h"
#include "inode.h"
#include "dir.h"
#include "stdint.h"
#include "list.h"
#include "string.h"
#include "ide.h"
#include "global.h"
#include "debug.h"
#include "memory.h"
#include "file.h"
#include "printk.h"

#define FS_MAGIC 0x20220221

struct partition* cur_part;

static void mount_partition(struct partition* part) {
    cur_part = part;
    struct disk* hd = cur_part->my_disk;

    struct super_block* sb_buf = (struct super_block*)sys_malloc(SECTOR_SIZE);
    ASSERT(sb_buf != NULL);
    memset(sb_buf, 0, SECTOR_SIZE);
    disk_read(hd, part->start_lba + 1, sb_buf, 1);
    cur_part->sb = sb_buf;

    part->block_bitmap.bits = (uint8_t*)sys_malloc(sb_buf->block_bitmap_sects * SECTOR_SIZE);
    part->block_bitmap.btmap_bytes_len = sb_buf->block_bitmap_sects * SECTOR_SIZE;
    disk_read(hd, sb_buf->block_bitmap_lba, part->block_bitmap.bits, sb_buf->block_bitmap_sects);

    part->inode_bitmap.bits =  (uint8_t*)sys_malloc(sb_buf->inode_bitmap_sects * SECTOR_SIZE);
    part->inode_bitmap.btmap_bytes_len = sb_buf->inode_bitmap_sects * SECTOR_SIZE;
    disk_read(hd, sb_buf->inode_bitmap_lba, part->inode_bitmap.bits, sb_buf->inode_bitmap_sects);
    
    list_init(&part->open_inodes);
    printk("mount %s done!\n", part->name);
}

void partition_format(struct partition* part) {
    /*
     * 分区内结构如下
     * | 操作系统引导块 | 超级块 | 空闲块位图 | inode位图 | inode数组 | 根目录 | 空闲块 |
     */
    uint32_t boot_sector_sects = 1;
    uint32_t super_block_sects = 1;
    uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, SECTOR_SIZE*8);
    uint32_t inode_table_sects = DIV_ROUND_UP( ((sizeof(struct inode)) * MAX_FILES_PER_PART), SECTOR_SIZE);
    uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
    uint32_t free_sects = part->sector_cnt - used_sects;

    uint32_t block_bitmap_sects = DIV_ROUND_UP(free_sects, SECTOR_SIZE*8);
    uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, SECTOR_SIZE*8);

    /* 超级块部分 */
    struct super_block *sb = (struct super_block*)sys_malloc(sizeof(struct super_block));
    sb->magic = FS_MAGIC;
    sb->sec_cnt = part->sector_cnt;;
    sb->inode_cnt = MAX_FILES_PER_PART;
    sb->part_lba_base = part->start_lba;

    sb->block_bitmap_lba = sb->part_lba_base + 2; // 第0块是引导块，第一块是超级块
    sb->block_bitmap_sects = block_bitmap_sects;

    sb->inode_bitmap_lba = sb->block_bitmap_lba + block_bitmap_sects;
    sb->inode_bitmap_sects = inode_bitmap_sects;

    sb->inode_table_lba = sb->inode_bitmap_lba + inode_bitmap_sects;
    sb->inode_table_sects = inode_table_sects;

    sb->data_start_lba = sb->inode_table_lba + inode_table_sects;
    sb->root_inode_no = 0;
    sb->dir_entry_size = sizeof(struct dir_entry);
    printk("%s info:\n", part->name);
    printk("   magic:0x%x\n   part_lba_base:0x%x\n   all_sectors:0x%x\n   inode_cnt:0x%x\n   block_bitmap_lba:0x%x\n   block_bitmap_sectors:0x%x\n   inode_bitmap_lba:0x%x\n   inode_bitmap_sectors:0x%x\n   inode_table_lba:0x%x\n   inode_table_sectors:0x%x\n   data_start_lba:0x%x\n", 
        sb->magic, 
        sb->part_lba_base, 
        sb->sec_cnt, 
        sb->inode_cnt, 
        sb->block_bitmap_lba, 
        sb->block_bitmap_sects, 
        sb->inode_bitmap_lba, 
        sb->inode_bitmap_sects, 
        sb->inode_table_lba, 
        sb->inode_table_sects, 
        sb->data_start_lba);
    struct disk* hd = part->my_disk;
    disk_write(hd, part->start_lba + 1, sb, 1);
    printk("  super_block_lba:0x%x\n", part->start_lba + 1);

    uint32_t buf_size = SECTOR_SIZE * (inode_table_sects > block_bitmap_sects? inode_table_sects: block_bitmap_sects);
    uint8_t* buf = (uint8_t *)sys_malloc(buf_size);
    // block_bitmap => sb->block_bitmap_lba
    // 未对齐多出来的部分不做处理，可能会出现越界的情况，先不管。FIXME
    /* 空闲块位图部分 */
    buf[0] |=0x01;  
    disk_write(hd, sb->block_bitmap_lba, buf, sb->block_bitmap_sects);

    /* inode 位图部分 */
    memset(buf, 0, buf_size);
    buf[0] |= 0x1;
    disk_write(hd, sb->inode_bitmap_lba, buf, sb->inode_bitmap_sects);

    /* inode 表部分，里面包括了inode 信息，包括 inode 号，文件大小以及内容存放在哪里。*/
    memset(buf, 0, buf_size);
    struct inode* i = (struct inode*)buf;
    i->i_size = sb->dir_entry_size * 2; // . and ..
    i->i_no = 0;
    i->i_sectors[0] = sb->data_start_lba;
    disk_write(hd, sb->inode_table_lba, buf, sb->inode_table_sects);

     
    /* 数据部分，写入 inode=0 的数据，其包括两个项目，"." 和 ".."，都指向 inode 0，属性都为目录 */
    memset(buf, 0, buf_size);
    struct dir_entry* p_de = (struct dir_entry*)buf;
    memcpy(p_de->filename, ".", 1);
    p_de->i_no = 0;
    p_de->f_type = FT_DIRECTORY;
    p_de++;

    memcpy(p_de->filename, "..", 2);
    p_de->i_no = 0;
    p_de->f_type = FT_DIRECTORY;

    disk_write(hd, sb->data_start_lba, buf, 1);
    
    printk("  root_dir_lba:%d\n", sb->data_start_lba);
    printk("%s format done\n", part->name);
    sys_free(sb);
    sys_free(buf);
}

static char* path_parse(char* pathname, char* name_store) {
    /*
        path_parse 返回结果：
        /a/b/c -> ret: b/c name: a
        b/c -> ret: c name: b
        c -> ret: NULL name: c
        c/ -> ret: / name: c
        c// -> ret: // name: c
        c//. -> ret: //. name: c
        //. -> ret: NULL name: .
        / -> ret: NULL name: NULL
        //// -> ret: NULL name: NULL
        '' -> ret: NULL name: NULL
    */
    
    // 跳过所有开头的 /，譬如 //////a//b/c，变为 a//b/c，再进行后续处理
    if(pathname[0] == '/') while(*(++pathname) == '/');
    while(*pathname != '/' && *pathname !=0) {
        *name_store++ = *pathname++;
    }
    // name_store 原来有数据也没关系，不依赖外部，减少埋雷
    *name_store = 0;
    return pathname;
}

int32_t path_depth_cnt(char* pathname) {
    ASSERT(pathname != NULL);
    char* p = pathname;
    char name[MAX_FILE_NAME_LEN];
    uint32_t depth = 0;

    p = path_parse(p, name);
    while(name[0]) {
        depth++;
        if(p) p = path_parse(p, name);
    }
    return depth;
}

static int32_t search_file(const char* pathname, struct path_search_record* search_record) {
    struct dir_entry dir_e;
    struct dir* parent_dir = &root_dir;
    uint32_t parent_inode_no = 0;
    char* sub_path = (char*)pathname;

    // 要求 pathname 以 '/' 开头
    ASSERT(pathname[0] == '/');
    // 先跳过所有的 '/'，第一个必须为 ‘/’，否则第一个跳过的不是 ‘/’，就是错误的。
    while(*(++sub_path) == '/');

    // 最开始的时候是 '/'，根是特殊的，需要手动初始化.
    search_record->parent_dir = &root_dir;
    search_record->file_type = FT_DIRECTORY;
    search_record->search_path[0] = 0;

    // 去掉开头的 '/' 后，就没其他字符了，那么就是 '/' 了
    if(sub_path[0] == 0) { return 0; }

    char name[MAX_FILE_NAME_LEN] = {0};
    sub_path = path_parse(sub_path, name);

    while(name[0]) {
        // 记录一下当前处理的路径是什么
        strcat(search_record->search_path, "/");
        strcat(search_record->search_path, name);
        ASSERT(strlen(search_record->search_path) < 512);

        if(!search_dir_entry(cur_part, parent_dir, name, &dir_e)) {
            // 没找到该 name，提前终止。
            // search_record->parent_dir 不关掉，后面创建文件要用
            return -1;
        }

        // 找到了文件，就结束。有可能是提前结束。先不管。
        if(FT_REGULAR == dir_e.f_type) {
            search_record->file_type = FT_REGULAR;
            return dir_e.i_no;
        }

        // 目录的情况，继续往下找
        if(FT_DIRECTORY == dir_e.f_type) {
            parent_inode_no = parent_dir->inode->i_no;
            dir_close(parent_dir);
            parent_dir = dir_open(cur_part, dir_e.i_no);
            search_record->parent_dir = parent_dir;
            sub_path = path_parse(sub_path, name);
            continue;
        }
    } 

    // 如果最后找到的是目录，那么会到这里，这个时候要更新一下其父目录
    dir_close(search_record->parent_dir);
    search_record->parent_dir = dir_open(cur_part, parent_inode_no);
    search_record->file_type = FT_DIRECTORY;
    return dir_e.i_no;
}

void filesystem_init() {
    struct super_block* sb_buf = (struct super_block*)sys_malloc(SECTOR_SIZE);
    if(sb_buf == NULL) PANIC("alloc memory failed!");
    struct disk* hd = &channels[0].devices[1];
    struct partition* part = hd->primary;
    ASSERT(part->sector_cnt !=0);
    memset(sb_buf, 0, SECTOR_SIZE);
    disk_read(hd, part->start_lba + 1, sb_buf, 1);
    if(sb_buf->magic == FS_MAGIC) {
        printk("%s has filesystem\n", part->name);
    } else {
        printk("formatting %s's partition %s.....\n", hd->name, part->name);
        partition_format(part);
    }
    sys_free(sb_buf);
    mount_partition(part);
    open_root_dir(cur_part);

    uint32_t fd_idx = 0;
    for(fd_idx = 0; fd_idx < MAX_FILE_OPEN; fd_idx ++) {
        file_table[fd_idx++].fd_inode == NULL;
    }
}

int32_t sys_open(const char* pathname, uint8_t flag) {
    int32_t fd = -1;

    // 先看下该文件是否已经存在
    struct path_search_record search_record;
    memset(&search_record, 0, sizeof(struct path_search_record));
    int32_t inode_no = search_file(pathname, &search_record);

    // 先看一下目录层数对不对
    if(path_depth_cnt(search_record.search_path) != path_depth_cnt((char*)pathname)) {
        printk("cannot access %s: Not a directory, subpath %s not exist\n", \
            pathname, search_record.search_path);
        dir_close(search_record.parent_dir);
        return -1;
    }

    if(inode_no == -1) {
        // 文件不存在的情况
        if(flag & O_CREAT) {
            printk("create %s\n", strrchr(pathname, '/') + 1);
            fd = file_create(search_record.parent_dir, strrchr(pathname, '/') + 1, flag);
            goto sys_open_ok;
        }
        printk("fail to open not exists path %s\n", pathname);
        goto sys_open_err;
    }

    // 文件存在的情况
    if(flag & O_CREAT) {
        printk("create on already exists path %s\n", pathname);
        goto sys_open_err;
    }

    if(FT_DIRECTORY == search_record.file_type) {
        printk("Is a directory %s\n", pathname);
        goto sys_open_err;
    }
    fd = file_open(inode_no, flag);

sys_open_ok:
    dir_close(search_record.parent_dir);
    return fd;

sys_open_err:
    dir_close(search_record.parent_dir);
    return -1;    
}

/* 将文件描述符转化为文件表的下标 */
static uint32_t fd_local2global(uint32_t local_fd) {
   struct task_struct* cur = running_thread();
   int32_t global_fd = cur->fd_table[local_fd];  
   ASSERT(global_fd >= 0 && global_fd < MAX_FILE_OPEN);
   return (uint32_t)global_fd;
} 

int32_t sys_close(int32_t fd) {
    int32_t ret = -1;
    if(fd > 2) {
        uint32_t global_fd = fd_local2global(fd);
        ret = file_close(&file_table[global_fd]);
        running_thread()->fd_table[fd] = -1;
    }
    return ret;
}

/*
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
*/


