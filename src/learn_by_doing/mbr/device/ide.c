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

struct partition_table {
    uint8_t bootable;
    uint8_t start_head;
    uint8_t start_sector;
    uint8_t start_chs;
    uint8_t fs_type;
    uint8_t end_head;
    uint8_t end_sector;
    uint8_t end_chs;
    uint32_t start_lba;
    uint32_t sector_cnt;
} __attribute__ ((packed));

static void select_disk(struct disk* hd) {
    outb(reg_dev(hd->my_channel), hd->dev_no*0x10 + BIT_DEV_MBS);
}

static void cmd_out(struct ide_channel* channel, uint8_t cmd) {
    channel->expecting_intr = true;
    outb(reg_cmd(channel), cmd);
}

static void set_disk_sector(struct disk* hd, uint32_t lba, uint8_t sector_cnt) {
    struct ide_channel* channel = hd->my_channel;
    outb(reg_sect_cnt(channel), sector_cnt);
    outb(reg_lba_l(channel), lba);
    outb(reg_lba_m(channel), lba>>8);
    outb(reg_lba_h(channel), lba>>16);
    uint32_t data = (lba >> 24) | BIT_DEV_LBA | (hd->dev_no*0x10 + BIT_DEV_MBS);
    outb(reg_dev(channel), data);
}

void intr_hd_handler(uint8_t irq_no) {
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    uint8_t ch_no = irq_no - 0x2e;
    struct ide_channel* channel = &channels[ch_no];
    ASSERT(channel->irq_no == irq_no);

    if(channel->expecting_intr) {
        channel->expecting_intr = false;
        sema_up(&channel->disk_done);
        inb(reg_status(channel));
    }
}

static bool busy_wait(struct disk* hd) {
    int max_wait=3000, interval = 10;
    struct ide_channel* channel = hd->my_channel;

    for(int i = 0; i < max_wait; i += interval) {
        int status = inb(reg_status(channel));
        if(status & BIT_STAT_BSY) {
            mtime_sleep(interval);
        } else {
            return (status & BIT_STAT_DRQ);
        }
    }
    return false;
}

static void read_buffer(struct disk* hd, void* buf, uint8_t cnt) {
    uint32_t size = 512 * (cnt==0? 256: cnt);
    insw(reg_data(hd->my_channel), buf, size / 2);
}

static char* le2be(const char* src, char* buf, uint32_t len) {
    uint32_t idx;
    for(idx = 0; idx < len; idx += 2) {
        buf[idx + 1] = *src++;
        buf[idx] = *src++;
    }
    buf[idx] = 0;
    return buf;
}

void disk_read(struct disk* hd, uint32_t lba, void* buffer, uint32_t sector_cnt) {
    uint32_t sector_done = 0;
    uint32_t sector_size = 0;

    lock_acquire(&hd->my_channel->lock);

    do {
        sector_size = sector_cnt - sector_done;
        sector_size = sector_size > 256? 256: sector_size;

        set_disk_sector(hd, lba + sector_done, sector_size);
        printk("read disk(%s): %d\n", hd->name, sector_size);

        cmd_out(hd->my_channel, CMD_READ_SECTOR);
        sema_down(&hd->my_channel->disk_done);
        printk("read disk(%s): before sema\n", hd->name);

        if(!busy_wait(hd)) {
            char error[64];
            sprintf(error, 
                    "Read %s addr:0x%x, sec: %d failed!!!\n", 
                    hd->name, lba, sector_cnt);
            PANIC(error);
        }
        read_buffer(hd, (void*)((uint32_t)buffer + sector_done * 512), sector_size);
        sector_done += sector_size;
    } while(sector_done < sector_cnt);

    lock_release(&hd->my_channel->lock);
}

static void identify_disk(struct disk* hd) {
    char id_info[512];
    struct ide_channel* channel = hd->my_channel;
    select_disk(hd);
    cmd_out(channel, CMD_IDENTIFY);
    sema_down(&channel->disk_done);

    if(!busy_wait(hd)) {
        char error[64];
        sprintf(error, "%s identify failed!!!\n", hd->name);
        printk(error);
        return;
        PANIC(error);
    }

    printk("  identify disk: %s\n", hd->name);
    read_buffer(hd, id_info, 1);
    char buf[64];
    printk("    SN: %s\n", le2be(&id_info[20], buf, 20));
    printk("    MODULE: %s\n", le2be(&id_info[27*2], buf, 40));
    uint32_t sector_cnt = *((uint32_t*)&id_info[120]);
    printk("    SECTORS: %d\n", sector_cnt);
    printk("    CAPACITY: %dMiB\n", sector_cnt*512/ 1024/1024);

    char* p = syscall_malloc(262144);
    disk_read(hd, 0, p, 1);
}

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
        register_handler(channel->irq_no, intr_hd_handler);
        int dev_no = 0;
        while(dev_no < 2) {
            struct disk* hd = &channel->devices[dev_no];
            hd->my_channel = channel;
            hd->dev_no = dev_no;
            sprintf(hd->name, "hd%c", 'a' + i*2 + dev_no);
            identify_disk(hd);
            dev_no++;
        }
    }
    printk("ide_init end\n");
}

