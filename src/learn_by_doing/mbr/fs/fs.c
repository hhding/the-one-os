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

#define FS_MAGIC 0x20220225

void partition_format(struct partition* part) {
    uint32_t boot_sector_sects = 1;
    uint32_t super_block_sects = 1;
    uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, SECTOR_SIZE);
    uint32_t inode_table_sects = DIV_ROUND_UP( ((sizeof(struct inode)) * MAX_FILES_PER_PART), SECTOR_SIZE);
    uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
    uint32_t free_sects = part->sector_cnt - used_sects;

    uint32_t block_bitmap_sects = DIV_ROUND_UP(free_sects, BLOCK_SIZE);
    uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BLOCK_SIZE);

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
    printk("   magic:0x%x\n   part_lba_base:0x%x\n   all_sectors:0x%x\n   inode_cnt:0x%x\n   block_bitmap_lba:0x%x\n   block_bitmap_sectors:0x%x\n   inode_bitmap_lba:0x%x\n   inode_bitmap_sectors:0x%x\n   inode_table_lba:0x%x\n   inode_table_sectors:0x%x\n   data_start_lba:0x%x\n", sb->magic, sb->part_lba_base, sb->sec_cnt, sb->inode_cnt, sb->block_bitmap_lba, sb->block_bitmap_sects, sb->inode_bitmap_lba, sb->inode_bitmap_sects, sb->inode_table_lba, sb->inode_table_sects, sb->data_start_lba);
    struct disk* hd = part->my_disk;
    disk_write(hd, part->start_lba+1, sb, 1);

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
}

/*
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
*/


