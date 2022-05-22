#include "stdint.h"
#include "ide.h"
#include "dir.h"
#include "global.h"
#include "super_block.h"
#include "memory.h"
#include "interrupt.h"
#include "file.h"
#include "printk.h"
#include "string.h"
#include "debug.h"
#include "timer.h"

#define MAX_FILE_OPEN 32    // 系统可打开的最大文件数

struct file file_table[MAX_FILE_OPEN];

int32_t test_and_set_bitmap(struct bitmap* a_bitmap) {
  int32_t idx = bitmap_scan(a_bitmap, 1);
   if(idx != -1) {
      bitmap_set(a_bitmap, idx, 1);
   }
   return idx;
}

int32_t inode_bitmap_alloc(struct partition* part) {
   return test_and_set_bitmap(&part->inode_bitmap); 
}

int32_t block_bitmap_alloc(struct partition* part) 
{
   uint32_t idx = test_and_set_bitmap(&part->block_bitmap); 
   if(idx == -1) return -1;
   return (part->sb->data_start_lba + idx);
}


void bitmap_sync(struct partition* part, uint32_t bit_idx, uint8_t btmp) {
   uint32_t offset_sec = bit_idx / (8*512);
   uint32_t offset_size = offset_sec * BLOCK_SIZE;
   uint32_t sec_lba = 0;
   uint8_t* bitmap_offset = NULL;

   switch (btmp)
   {
   case INODE_BITMAP:
      sec_lba = part->sb->inode_bitmap_lba + offset_sec;
      bitmap_offset = part->inode_bitmap.bits + offset_size;
      break;
   case BLOCK_BITMAP:
      sec_lba = part->sb->block_bitmap_lba + offset_sec;
      bitmap_offset = part->block_bitmap.bits + offset_size;
      break;
   }
   disk_write(part->my_disk, sec_lba, bitmap_offset, 1);
}

int32_t get_free_slot_in_global(void) {
   for(int idx=0; idx < MAX_FILE_OPEN; idx++) {
      if(file_table[idx].fd_inode == NULL) return idx;
   }
   printk("exceed max open files\n");
   return -1;
}

int32_t pcb_fd_install(int32_t globa_fd_idx) {
   struct task_struct* cur = running_thread();
   
   for(int32_t fd_idx = 3; fd_idx < MAX_FILES_OPEN_PER_PROC; fd_idx++) {
      if(cur->fd_table[fd_idx] == -1) {
         cur->fd_table[fd_idx] = globa_fd_idx;
         return fd_idx;
      }
   }
   printk("exceed max open files_per_proc\n");
   return -1;
}

int32_t file_create(struct dir* parent_dir, char* filename, uint8_t flag) {
   void* io_buf = sys_malloc(1024);
   if(io_buf == NULL) {
      printk("file_create: sys_malloc for io_buf failed\n");
      return -1;
   }

   uint8_t rollback_step = 0;

   int32_t inode_no = inode_bitmap_alloc(cur_part);
   if(inode_no == -1) {
      printk("file_create: allocate inode failed\n");
      return -1;
   }

   struct inode* new_file_inode = (struct inode*)sys_malloc(sizeof(struct inode));
   if(new_file_inode == NULL) {
      printk("file_create: sys_malloc for inode failed\n");
      rollback_step = 1;
      goto rollback;
   }
   inode_init(inode_no, new_file_inode);
   int fd_idx = get_free_slot_in_global();
   if(fd_idx == -1) {
      printk("exceed max open files\n");
      rollback_step = 2;
      goto rollback;
   }

   file_table[fd_idx].fd_inode = new_file_inode;
   file_table[fd_idx].fd_pos = 0;
   file_table[fd_idx].fd_flag = flag;
   file_table[fd_idx].fd_inode->write_deny = false;

   struct dir_entry new_dir_entry;
   memset(&new_dir_entry, 0, sizeof(struct dir_entry));

   create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry);
   if(!sync_dir_entry(parent_dir, &new_dir_entry, io_buf)) {
      printk("sync dir_entry to disk failed\n");
      rollback_step = 3;
      goto rollback;
   }
   memset(io_buf, 0, 1024);
   inode_sync(cur_part, parent_dir->inode, io_buf);
   inode_sync(cur_part, new_file_inode, io_buf);
   bitmap_sync(cur_part, inode_no, INODE_BITMAP);
   list_push(&cur_part->open_inodes, &new_file_inode->inode_tag);
   new_file_inode->i_open_cnt = 1;
   sys_free(io_buf);
   return pcb_fd_install(fd_idx);

rollback:
    switch (rollback_step) {
    case 3:
      memset(&file_table[fd_idx], 0, sizeof(struct file));
    case 2:
      sys_free(new_file_inode);
    case 1:
      bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
      break;
    }
    sys_free(io_buf);
    return -1;
}

int32_t file_open(uint32_t inode_no, uint8_t flag) {
   int fd_idx = get_free_slot_in_global();
   if(fd_idx == -1) {
      printk("exceed max open files\n");
      return -1;
   }
   file_table[fd_idx].fd_inode = inode_open(cur_part, inode_no);
   file_table[fd_idx].fd_pos = 0;
   file_table[fd_idx].fd_flag = flag;
   bool* write_deny = &file_table[fd_idx].fd_inode->write_deny;
   if(flag == O_WRONLY || flag == O_RDWR) {
      if(!(*write_deny)) {
         *write_deny = true;
      } else {
         printk("file can't be write now, try again later\n");
         return -1;
      }
   }
   return pcb_fd_install(fd_idx);
}

int32_t file_close(struct file* file) {
   if(file == NULL) {
      return -1;
   }
   // ???
   file->fd_inode->write_deny = false;
   inode_close(file->fd_inode);
   file->fd_inode = NULL;
   return 0;
}

int32_t allocate_block() {
   int32_t block_lba = block_bitmap_alloc(cur_part);
   ASSERT(block_lba != -1);
   uint32_t block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
   bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
   printk("allocate_block: lba %d\n", block_lba);
   return block_lba;
}

static int32_t load_or_alloc_inode_block(struct inode* fd_inode, uint32_t block_idx, uint32_t* all_blocks, void* io_buf) {
   int32_t block_lba = all_blocks[block_idx];
   ASSERT(block_lba >= 0);
   if(block_lba == 0) {
      block_lba = allocate_block();
      //printk("load_or_allocate_block: allocate lba %d, block_idx: %d\n", block_lba, block_idx);
      if(block_idx < 12) fd_inode->i_sectors[block_idx] = block_lba;
      all_blocks[block_idx] = block_lba;
      memset(io_buf, 0, BLOCK_SIZE);
      if(block_idx >= 12) {
         printk("sync indirect block %d to lba %d\n", block_idx, fd_inode->i_sectors[12]);
         disk_write(cur_part->my_disk, fd_inode->i_sectors[12], all_blocks + 12, 1);
      }
   } else {
      disk_read(cur_part->my_disk, block_lba, io_buf, 1);
   }
   return 0;
}

int32_t file_write(struct file* file, const void* buf, uint32_t count) {
   // 覆盖写入不应该有问题，这里判断需要优化
   if((file->fd_pos + count) > (BLOCK_SIZE*140)) {
      printk("file_write: exceed max file_size 71680 bytes\n");
      return -1;
   }
   uint8_t* io_buf = sys_malloc(BLOCK_SIZE);
   if(io_buf == NULL) {
      printk("file_write: sys_malloc for io_buf failed\n");
      return -1;
   }
   uint32_t start_sec = file->fd_pos / BLOCK_SIZE;
   uint32_t start_offset = file->fd_pos % BLOCK_SIZE;
   uint32_t end_sec = (file->fd_pos + count) / BLOCK_SIZE;
   uint32_t end_offset = (file->fd_pos + count) % BLOCK_SIZE;
   uint32_t* all_blocks = (uint32_t*)sys_malloc(BLOCK_SIZE + 48);
   int32_t block_lba = -1;

   for(int i = 0; i < 12; i++) {
      all_blocks[i] = file->fd_inode->i_sectors[i];
   }

   if(end_sec >= 12) {
      if(file->fd_inode->i_sectors[12] == 0) {
         block_lba = allocate_block();
         file->fd_inode->i_sectors[12] = block_lba;
         printk("allocate new block for indirect: lba: %d\n", file->fd_inode->i_sectors[12]);
         memset(all_blocks + 12, 0, BLOCK_SIZE);
      } else 
         //printk("read for indirect: lba: %d\n", file->fd_inode->i_sectors[12]);
         disk_read(cur_part->my_disk, file->fd_inode->i_sectors[12], all_blocks + 12, 1);
   }

   uint32_t bytes_written = 0;
   if(start_sec == end_sec) {
      load_or_alloc_inode_block(file->fd_inode, start_sec, all_blocks, io_buf);
      // printk("file_write: lba: %d, offset: %d, cnt: %d block0: %d\n", all_blocks[start_sec], start_offset, count, file->fd_inode->i_sectors[0]);
      memcpy(io_buf + start_offset, buf, count);
      disk_write(cur_part->my_disk, all_blocks[start_sec], io_buf, 1);
      bytes_written = count;
   } else {
      // printk("multi sector, file_write: start: %d end:%d\n", start_sec, end_sec);
      // 跨扇区的情况
      // 先处理第一个扇区
      load_or_alloc_inode_block(file->fd_inode, start_sec, all_blocks, io_buf);
      memcpy(io_buf + start_offset, buf, BLOCK_SIZE - start_offset);
      disk_write(cur_part->my_disk, all_blocks[start_sec], io_buf, 1);

      bytes_written = BLOCK_SIZE - start_offset;
      for(int32_t idx = start_sec + 1; idx < end_sec; idx++) {
         load_or_alloc_inode_block(file->fd_inode, idx, all_blocks, io_buf);
         memcpy(io_buf, buf + bytes_written, BLOCK_SIZE);
         disk_write(cur_part->my_disk, all_blocks[idx], io_buf, 1);
         bytes_written += BLOCK_SIZE;
      }
      // 先处理最后一个扇区
      load_or_alloc_inode_block(file->fd_inode, end_sec, all_blocks, io_buf);
      memcpy(io_buf, buf + bytes_written, end_offset);
      disk_write(cur_part->my_disk, all_blocks[end_sec], io_buf, 1);
      bytes_written += end_offset;
   }

   file->fd_pos = file->fd_pos + bytes_written;
   if(file->fd_pos > file->fd_inode->i_size) file->fd_inode->i_size = file->fd_pos;
   //printk("file_write: inode: %d, block0: %d, size: %d\n", file->fd_inode->i_no, file->fd_inode->i_sectors[0], file->fd_inode->i_size);
   inode_sync(cur_part, file->fd_inode, io_buf);
   sys_free(all_blocks);
   sys_free(io_buf);

   return bytes_written;
}

int32_t file_read(struct file* file, void* buf, uint32_t count) {
   uint8_t* io_buf = (uint8_t*)sys_malloc(BLOCK_SIZE);
   if(io_buf == NULL) {
      printk("file_write: sys_malloc for io_buf failed\n");
      goto file_read_error;
   }

   uint32_t size = count, size_left;

   if(count < 0) return -1;

   if(file->fd_pos > file->fd_inode->i_size) return -1;
   if(file->fd_pos + count > file->fd_inode->i_size) {
      size = file->fd_inode->i_size - file->fd_pos;
   }

   // printk("file_read: ino: %d, pos: %d, file_size:%d, size: %d\n", file->fd_inode->i_no, file->fd_pos, file->fd_inode->i_size, size);
   if(size == 0) return 0;

   uint32_t start_sec = file->fd_pos / BLOCK_SIZE;
   uint32_t start_offset = file->fd_pos % BLOCK_SIZE;
   uint32_t end_sec = (file->fd_pos + size) / BLOCK_SIZE;
   uint32_t end_offset = (file->fd_pos + size) % BLOCK_SIZE;
   uint32_t* all_blocks = (uint32_t*)sys_malloc(BLOCK_SIZE + 48);

   if(all_blocks == NULL) {
      printk("file_write: sys_malloc for all_blocks failed\n");
      goto file_read_error;
   }
   int32_t block_lba = -1;

   for(int i = 0; i < 12; i++) {
      all_blocks[i] = file->fd_inode->i_sectors[i];
   }

   if(end_sec > 12) {
      block_lba = file->fd_inode->i_sectors[12];
      if(block_lba == 0) {
         memset(all_blocks + 12, 0, BLOCK_SIZE);
      } else 
         disk_read(cur_part->my_disk, file->fd_inode->i_sectors[12], all_blocks + 12, 1);
   }

   int32_t bytes_read = 0;
   if(start_sec == end_sec) {
      // printk("file_read: %d -> %d\n", start_sec, all_blocks[start_sec]);
      disk_read(cur_part->my_disk, all_blocks[start_sec], io_buf, 1);
      memcpy(buf, io_buf+start_offset, size);
      return size;
   } else {
      // 跨扇区的情况
      // 先处理第一个扇区
      disk_read(cur_part->my_disk, all_blocks[start_sec], io_buf, 1);
      memcpy(buf, io_buf + start_offset, BLOCK_SIZE - start_offset);
      bytes_read = BLOCK_SIZE - start_offset;

      for(int32_t idx = start_sec + 1; idx < end_sec; idx++) {
         disk_read(cur_part->my_disk, all_blocks[idx], io_buf, 1);
         memcpy(buf + bytes_read, io_buf, BLOCK_SIZE);
         bytes_read += BLOCK_SIZE;
      }
      // 先处理最后一个扇区
      disk_read(cur_part->my_disk, all_blocks[end_sec], io_buf, 1);
      memcpy(buf + bytes_read, io_buf, end_offset);
      bytes_read += end_offset;
   }

   sys_free(all_blocks);
   sys_free(io_buf);
   file->fd_pos = file->fd_pos + bytes_read;
   return bytes_read;

file_read_error:
   if(all_blocks != NULL) sys_free(all_blocks);
   if(io_buf != NULL) sys_free(io_buf);
   return -1;
}
