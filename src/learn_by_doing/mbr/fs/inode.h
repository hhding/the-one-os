#ifndef __FS_INODE_H
#define __FS_INODE_H
#include "stdint.h"
#include "list.h"
#include "ide.h"

struct inode {
    uint32_t i_no;
    uint32_t i_size;    // 文件大小或者目录个数
    uint32_t i_open_cnt;    // 当前有多少人打开了该文件
    bool write_deny;
    // 0-11 直接块，12 指向一级间接块指针
    uint32_t i_sectors[13];
    struct list_elem inode_tag;
};

struct inode* inode_open(struct partition* part, uint32_t inode_no);
void inode_sync(struct partition* part, struct inode* inode, void* io_buf);
void inode_init(uint32_t inode_no, struct inode* new_inode);
void inode_close(struct inode* inode);
void inode_release(struct partition* part, uint32_t inode_no);
void inode_delete(struct partition* part, uint32_t inode_no, void* io_buf);
#endif
