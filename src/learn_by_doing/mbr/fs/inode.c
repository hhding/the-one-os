#include "inode.h"
#include "stdint.h"
#include "list.h"
#include "ide.h"
#include "string.h"
#include "super_block.h"
#include "interrupt.h"
#include "memory.h"
#include "debug.h"
#include "file.h"
#include "printk.h"

#define SECTOR_SIZE 512

struct inode_position {
    bool cross_sector;
    uint32_t sec_lba;
    uint32_t sec_offset;
};

static void inode_locate(struct partition* part, uint32_t inode_no, struct inode_position* pos) {
    pos->sec_lba = part->sb->inode_table_lba + (inode_no * sizeof(struct inode)) / SECTOR_SIZE;
    pos->sec_offset = (inode_no * sizeof(struct inode)) % SECTOR_SIZE;
    if(pos->sec_offset + sizeof(struct inode) > SECTOR_SIZE) {
        pos->cross_sector = true;
    }else{
        pos->cross_sector = false;
    }
}

struct inode* inode_open(struct partition* part, uint32_t inode_no) {
    ASSERT(inode_no < 4096);
    struct list_elem *elem = part->open_inodes.head.next;
    struct inode* inode_found;
    while(elem != &part->open_inodes.tail) {
        inode_found = elem2entry(struct inode, inode_tag, elem);
        if(inode_found->i_no == inode_no) {
            inode_found->i_open_cnt++;
            return inode_found;
        }
        elem = elem->next;
    }

    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    struct task_struct *cur = running_thread();
    uint32_t* pgdir = cur->pgdir;
    cur->pgdir = NULL;
    // 本来应该有个 kmalloc 函数来确保分配的内存在内核地址空间的。
    inode_found = (struct inode*)sys_malloc(sizeof(struct inode));
    cur->pgdir = pgdir;

    char* inode_buf;
    int sec_cnt = inode_pos.cross_sector? 2 : 1;
    inode_buf = (char*)sys_malloc(SECTOR_SIZE* sec_cnt);
    disk_read(part->my_disk, inode_pos.sec_lba, inode_buf, sec_cnt);
    memcpy(inode_found, inode_buf + inode_pos.sec_offset, sizeof(struct inode));
    inode_found->i_open_cnt = 1;
    list_push(&part->open_inodes, &inode_found->inode_tag);
    sys_free(inode_buf);
    return inode_found;
}

void inode_sync(struct partition* part, struct inode* inode, void* io_buf) {
    struct inode inode_ondisk;
    memcpy(&inode_ondisk, inode, sizeof(struct inode));
    inode_ondisk.i_open_cnt = 0;
    inode_ondisk.write_deny = false;
    inode_ondisk.inode_tag.prev = inode_ondisk.inode_tag.next = NULL;

    struct inode_position inode_pos;
    inode_locate(part, inode->i_no, &inode_pos);
    int sec_cnt = inode_pos.cross_sector? 2 : 1;
    char* inode_buf = (char*)io_buf;
    disk_read(part->my_disk, inode_pos.sec_lba, inode_buf, sec_cnt);
    memcpy(inode_buf + inode_pos.sec_offset, &inode_ondisk, sizeof(struct inode));
    disk_write(part->my_disk, inode_pos.sec_lba, inode_buf, sec_cnt);
}

void inode_init(uint32_t inode_no, struct inode* new_inode) {
    new_inode->i_no = inode_no;
    new_inode->i_size = 0;
    new_inode->i_open_cnt = 0;
    for(int idx=0; idx<13; idx++) {
        new_inode->i_sectors[idx] = 0;
    }
}

void inode_close(struct inode* inode) {
    enum intr_status old_status = intr_disable();
    if(--inode->i_open_cnt == 0) {
        list_remove(&inode->inode_tag);
        struct task_struct *cur = running_thread();
        uint32_t* pgdir = cur->pgdir;
        cur->pgdir = NULL;
        // 本来应该有个 kmalloc 函数来确保分配的内存在内核地址空间的。
        sys_free(inode);
        cur->pgdir = pgdir;
    }
    intr_set_status(old_status);
}

void inode_delete(struct partition* part, uint32_t inode_no, void* io_buf) {
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    char* inode_buf = (char*) io_buf;
    int sec_cnt = inode_pos.cross_sector? 2 : 1;
    disk_read(part->my_disk, inode_pos.sec_lba, inode_buf, sec_cnt);
    memset(inode_buf + inode_pos.sec_offset, 0, sizeof(struct inode));
    disk_write(part->my_disk, inode_pos.sec_lba, inode_buf, sec_cnt);
}

void inode_release(struct partition* part, uint32_t inode_no) {
    struct inode* inode_to_del = inode_open(part, inode_no);
    uint32_t block_bitmap_idx;
    uint32_t all_blocks[140] = {0};
    uint8_t block_idx = 0, block_cnt = 12;
    while(block_idx < 12) {
        all_blocks[block_idx] = inode_to_del->i_sectors[block_idx];
        block_idx++;
    }
    if(inode_to_del->i_sectors[12] != 0) {
        // 128 blocks + 12 blocks = 140
        disk_read(part->my_disk, inode_to_del->i_sectors[12], all_blocks + 12, 1);
        block_cnt = 140;
        // 本扇区先回收
        block_bitmap_idx = inode_to_del->i_sectors[12] - part->sb->data_start_lba;
        ASSERT(block_bitmap_idx > 0);
        bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
        bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
    }
    for(block_idx=0; block_idx < block_cnt; block_idx++) {
        if(all_blocks[block_idx] != 0) {
            block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;
            ASSERT(block_bitmap_idx > 0);
            bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
            bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
        }
    }
    bitmap_set(&part->inode_bitmap, inode_no, 0);
    bitmap_sync(cur_part, inode_no, INODE_BITMAP);
    inode_close(inode_to_del);
}

