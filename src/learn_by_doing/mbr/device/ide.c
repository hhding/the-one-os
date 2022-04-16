#include "ide.h"
#include "sync.h"
#include "io.h"
#include "stdio.h"
//#include "stdio-kernel.h"
#include "interrupt.h"
#include "memory.h"
#include "debug.h"
#include "printk.h"
#include "timer.h"
#include "string.h"
#include "list.h"

/* 定义硬盘各寄存器的端口号 */
#define reg_data(channel)    (channel->port_base + 0)
#define reg_error(channel)   (channel->port_base + 1)
#define reg_sect_cnt(channel)    (channel->port_base + 2)
#define reg_lba_l(channel)   (channel->port_base + 3)
#define reg_lba_m(channel)   (channel->port_base + 4)
#define reg_lba_h(channel)   (channel->port_base + 5)
#define reg_dev(channel)     (channel->port_base + 6)
#define reg_status(channel)  (channel->port_base + 7)
#define reg_cmd(channel)     (reg_status(channel))
#define reg_alt_status(channel)  (channel->port_base + 0x206)
#define reg_ctl(channel)     reg_alt_status(channel)

/* reg_status寄存器的一些关键位 */
#define BIT_STAT_BSY     0x80         // 硬盘忙
#define BIT_STAT_DRDY    0x40         // 驱动器准备好
#define BIT_STAT_DRQ     0x8          // 数据传输准备好了

/* device寄存器的一些关键位 */
#define BIT_DEV_MBS 0xa0        // 第7位和第5位固定为1
#define BIT_DEV_LBA 0x40
#define BIT_DEV_DEV 0x10

/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY       0xec     // identify指令
#define CMD_READ_SECTOR    0x20     // 读扇区指令
#define CMD_WRITE_SECTOR   0x30     // 写扇区指令

#define DISK_CNT_ADDR 0x475
struct ide_channel channels[2];

void ide_init() {
    printk("ide_init start\n");
    // 可以将其看作是一个8bit的指针，取其第一个值
    // uint8_t disk_cnt = ((uint8_t*)DISK_CNT_ADDR)[0];
    channels[0].port_base = 0x1f0;
    channels[0].irq_no = 0x20 + 14;
    channels[1].port_base = 0x170;
    channels[1].irq_no = 0x20 + 15;
    struct ide_channel* channel;
    for(int i=0; i<2; i++) {
        channel = &channels[i];
        channel->expecting_intr = false;
        lock_init(&channel->lock);
        sema_init(&channel->disk_done, 0);
        // register_handler(channel->irq_no, intr_hd_handler);
        int dev_no = 0;
        while(dev_no < 2) {
            struct disk* hd = &channel->devices[dev_no];
            hd->my_channel = channel;
            hd->dev_no = dev_no;
            sprintf(hd->name, "hd%c", 'a' + i*2 + dev_no);
            printk("%s\n", hd->name);
            dev_no++;
        }
    }
    printk("ide_init end\n");
}

