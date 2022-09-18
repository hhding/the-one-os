#include "dir.h"
#include "ide.h"
#include "super_block.h"
#include "file.h"
#include "debug.h"
#include "printk.h"
#include "string.h"

struct dir root_dir;

void open_root_dir(struct partition* part) {
    root_dir.inode = inode_open(part, part->sb->root_inode_no);
    root_dir.dir_pos = 0;
}

struct dir* dir_open(struct partition* part, uint32_t inode_no) {
    struct dir* pdir = (struct dir*)sys_malloc(sizeof(struct dir));
    pdir->inode = inode_open(part, inode_no);
    pdir->dir_pos = 0;
    return pdir;
}

void dir_close(struct dir* dir) {
    if(dir == &root_dir) return;
    inode_close(dir->inode);
    sys_free(dir);
}

bool search_dir_entry(struct partition* part, 
    struct dir* pdir, const char* name, struct dir_entry* dir_e) {
    uint32_t block_cnt = 140;
    uint32_t all_blocks[140] = {0};
    ASSERT(all_blocks != NULL);
    uint32_t block_idx;

    for(block_idx=0; block_idx<12; block_idx++) {
        all_blocks[block_idx] = pdir->inode->i_sectors[block_idx];
    }
    
    if(pdir->inode->i_sectors[12] != 0)  {
        disk_read(part->my_disk, pdir->inode->i_sectors[12], all_blocks + 12, 1);
    }

    uint8_t *buf = (uint8_t *)sys_malloc(SECTOR_SIZE);
    struct dir_entry *p_de = (struct dir_entry *)buf;
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    uint32_t dir_entry_cnt = SECTOR_SIZE / dir_entry_size;
    for(block_idx=0; block_idx<block_cnt; block_idx++) {
        if(all_blocks[block_idx] == 0) continue;
        disk_read(part->my_disk, all_blocks[block_idx], buf, 1);
        for (uint32_t dir_entry_idx = 0; dir_entry_idx < dir_entry_cnt; dir_entry_idx++) {
            if(!strcmp((p_de + dir_entry_idx)->filename, name)) {
                memcpy(dir_e, p_de + dir_entry_idx, dir_entry_size);
                sys_free(buf);
                return true;
            }
        }
        memset(buf, 0, SECTOR_SIZE);
    }
    sys_free(buf);
    return false;
}

void create_dir_entry(char* filename, uint32_t inode_no, uint8_t file_type, struct dir_entry* p_de) {
    ASSERT(strlen(filename) <= MAX_FILE_NAME_LEN);
    memcpy(p_de->filename, filename, strlen(filename));
    p_de->i_no = inode_no;
    p_de->f_type = file_type;
}

static int32_t find_free_dir_entry(struct partition* part, uint32_t lba, void* io_buf) {
    uint32_t dir_entrys_per_sec = SECTOR_SIZE / cur_part->sb->dir_entry_size;
    struct dir_entry* dir_e = (struct dir_entry*)io_buf;
    disk_read(cur_part->my_disk, lba, io_buf, 1);
    for (uint32_t dir_entry_idx = 0; dir_entry_idx < dir_entrys_per_sec; dir_entry_idx++) {
        if((dir_e + dir_entry_idx)->f_type == FT_UNKNOWN) {
            return dir_entry_idx;
        }
    }
    return -1;
}

uint32_t allocate_part_block(struct partition* part, void* io_buf) {
    int32_t block_lba = block_bitmap_alloc(part);
    ASSERT(block_lba != -1);
    bitmap_sync(cur_part, block_lba - part->sb->data_start_lba, BLOCK_BITMAP);
    memset(io_buf, 0, 512);
    // 有可能这个扇区是有数据的，先用全 0 清空它
    disk_write(cur_part->my_disk, block_lba, io_buf, 1);
    return block_lba;
}

// 从 parent_dir 找到空闲目录项，并且写到硬盘
bool sync_dir_entry(struct dir* parent_dir, struct dir_entry* p_de, void* io_buf) {
    struct inode* parent_inode = parent_dir->inode;
    struct dir_entry* dir_e = (struct dir_entry*)io_buf;
    int32_t block_lba;
    int32_t dir_entry_idx, idx;

    // 先从前面 12 个 direct block 里面找
    for (idx = 0; idx < 12; idx++) {
        block_lba = parent_inode->i_sectors[idx];
        if(block_lba == 0) {
            block_lba = allocate_part_block(cur_part, io_buf);
            parent_inode->i_sectors[idx] = block_lba;
        }
        dir_entry_idx = find_free_dir_entry(cur_part, block_lba, io_buf);
        if(dir_entry_idx != -1) goto do_sync_dir_entry;
    }

    // 加载 128 个 indirect block
    block_lba = parent_inode->i_sectors[12];
    if(block_lba == 0) {
        block_lba = allocate_part_block(cur_part, io_buf);
        parent_inode->i_sectors[idx] = block_lba;
    }

    uint32_t tier2_block_idx[128] = {0};
    disk_read(cur_part->my_disk, block_lba, tier2_block_idx, 1);
    // 在 128 个 indirect block里面找
    for (idx = 0; idx < 128; idx++) {
        block_lba = tier2_block_idx[idx];
        if(block_lba == 0) {
            block_lba = allocate_part_block(cur_part, io_buf);
            tier2_block_idx[idx] = block_lba;
            disk_write(cur_part->my_disk, parent_inode->i_sectors[12], tier2_block_idx, 1);
        }
        dir_entry_idx = find_free_dir_entry(cur_part, block_lba, io_buf);
        if(dir_entry_idx != -1) goto do_sync_dir_entry;
    }
    printk("directory is full\n");
    return false;

do_sync_dir_entry:
    memcpy(dir_e + dir_entry_idx, p_de, cur_part->sb->dir_entry_size);
    disk_write(cur_part->my_disk, block_lba, io_buf, 1);
    // 如果分配的目录项是原先删除的目录项，那么这里就不应该加。
    // 所以这里是不对的。FIXME
    parent_inode->i_size += cur_part->sb->dir_entry_size;
    return true;
}

bool delete_dir_entry(struct partition* part, struct dir* pdir, uint32_t inode_no, void* io_buf) {
    struct inode* dir_inode = pdir->inode;
    uint32_t block_idx = 0, all_blocks[140] = {0};
    uint32_t dir_entrys_per_sec = SECTOR_SIZE / cur_part->sb->dir_entry_size;
    struct dir_entry *dir_e = (struct dir_entry*)io_buf;
    struct dir_entry* dir_entry_found = NULL;
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    uint32_t dir_entry_cnt;

    for(block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = dir_inode->i_sectors[block_idx];
    }
    if(dir_inode->i_sectors[12] != 0) {
        disk_read(part->my_disk, dir_inode->i_sectors[12], all_blocks + 12, 1);
    }

    for(block_idx = 0; block_idx < 140; block_idx++) {
        dir_entry_cnt = 0;
        if(all_blocks[block_idx] == 0) continue;
        disk_read(part->my_disk, all_blocks[block_idx], io_buf, 1);
        for(uint32_t dir_entry_idx = 0; dir_entry_idx < dir_entrys_per_sec; dir_entry_idx++) {
            if((dir_e + dir_entry_idx)->f_type != FT_UNKNOWN) {
                dir_entry_cnt++;
                if((dir_e + dir_entry_idx)->i_no == inode_no) {
                    ASSERT(dir_entry_found == NULL);
                    dir_entry_found = dir_e + dir_entry_idx;
                }
            }
        }
        // 处理完当前这个块，如果找到了，那么就进行实际的删除动作
        if(dir_entry_found != NULL) goto do_delete_dir_entry;
    }
    return false;

do_delete_dir_entry:
    ASSERT(dir_entry_cnt >= 1);
    if(dir_entry_cnt == 1) {
        // 需要清空块的情况
        // 先回收当前块，数据就不管他了，bitmap 回收就结束了
        uint32_t block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;
        bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
        bitmap_sync(part, block_bitmap_idx, BLOCK_BITMAP);
        if(block_idx < 12) {
            dir_inode->i_sectors[block_idx] = 0;
        } else {
            all_blocks[block_idx] = 0;
            uint32_t indirect_blocks = 0;
            for(int indirect_block_idx = 12; indirect_block_idx < 140; indirect_block_idx++) {
                if(all_blocks[indirect_block_idx] != 0) indirect_blocks++;
            }
            if(indirect_blocks == 0) {
                block_bitmap_idx = dir_inode->i_sectors[12] - part->sb->data_start_lba;
                bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
                bitmap_sync(part, block_bitmap_idx, BLOCK_BITMAP);
                dir_inode->i_sectors[12] = 0;
            } else {
                // 上面的情况下，如果回收了，就没有必要再写一次了。
                disk_write(part->my_disk, dir_inode->i_sectors[12], all_blocks + 12, 1);
            }
        }
    } else {
        memset(dir_entry_found, 0, dir_entry_size);
        disk_write(part->my_disk, all_blocks[block_idx], io_buf, 1);
    }

    dir_inode->i_size -= dir_entry_size;
    memset(io_buf, 0, SECTOR_SIZE * 2);
    inode_sync(part, dir_inode, io_buf);
    return true;
}

// 返回目录中的下一个项
struct dir_entry* dir_read(struct dir* dir) {
    uint32_t all_blocks[140] = {0};
    uint32_t block_idx;
    struct dir_entry* dir_e = (struct dir_entry*)dir->dir_buf;
    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
    uint32_t dir_entrys_per_sec = SECTOR_SIZE / dir_entry_size;
    uint32_t cur_dir_entry_pos = 0;

    for(block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = dir->inode->i_sectors[block_idx];
    }
    if(dir->inode->i_sectors[12] != 0) {
        disk_read(cur_part->my_disk, dir->inode->i_sectors[12], all_blocks + 12, 1);
    }

    for(block_idx = 0; block_idx < 140; block_idx++) {
        if(dir->dir_pos > dir->inode->i_size) {
            return NULL;
        }
        if(all_blocks[block_idx] == 0) continue;
        disk_read(cur_part->my_disk, all_blocks[block_idx], dir_e, 1);
        for(uint32_t dir_idx = 0; dir_idx < dir_entrys_per_sec; dir_idx++) {
            //printk("block: %d, idx: %d, dir_pos: %d\n", block_idx, dir_idx, dir->dir_pos);
            if((dir_e + dir_idx)->f_type) {
                cur_dir_entry_pos += dir_entry_size;
                if(cur_dir_entry_pos <= dir->dir_pos) continue;
                ASSERT(dir->dir_pos + dir_entry_size == cur_dir_entry_pos)
                dir->dir_pos = cur_dir_entry_pos;
                return dir_e + dir_idx;
            }
        }
    }
    return NULL;
}

bool dir_is_empty(struct dir* dir) {
    return (dir->inode->i_size == cur_part->sb->dir_entry_size * 2);
}

int32_t dir_remove(struct dir* parent_dir, struct dir* child_dir) {
    struct inode* child_dir_inode = child_dir->inode;
    for(int block_idx = 1; block_idx < 13; block_idx++) {
        ASSERT(child_dir_inode->i_sectors[block_idx] == 0);
    }
    void* io_buf = sys_malloc(SECTOR_SIZE * 2);
    if(io_buf == NULL) {
        printk("dir_remove: malloc for io_buf failed\n");
        return -1;
    }
    delete_dir_entry(cur_part, parent_dir, child_dir_inode->i_no, io_buf);
    inode_release(cur_part, child_dir_inode->i_no);
    sys_free(io_buf);
    return 0;
}
