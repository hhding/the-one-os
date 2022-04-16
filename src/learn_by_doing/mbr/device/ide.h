#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "stdint.h"
#include "sync.h"
#include "list.h"
#include "bitmap.h"

struct partition {
    char name[8];
    uint32_t start_lba;
    uint32_t sector_cnt;
    struct disk* my_disk;
    struct list_elem part_tag;
    struct super_block* sb;
    struct bitmap block_bitmap;
    struct bitmap inode_bitmap;
    struct list open_inodes;
};


struct disk {
    char name[8];
    struct ide_channel* my_channel;
    uint8_t dev_no;
    struct partition primary[4];
};

struct ide_channel {
    char name[8];
    uint16_t port_base;
    uint8_t irq_no;
    struct lock lock;
    bool expecting_intr;
    struct semaphore disk_done;
    struct disk devices[2];     // 一个通道可以接主备两个盘
};

void intr_hd_handler(uint8_t irq_no);
void ide_init(void);
extern uint8_t channel_cnt;
extern struct ide_channel channels[];
extern struct list partition_list;
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);

#endif

