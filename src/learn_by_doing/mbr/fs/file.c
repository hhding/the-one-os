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

int32_t file_write(struct file* file, const void* buf, uint32_t count) {
   // 覆盖写入不应该有问题，这里判断需要优化
   if((file->fd_inode->i_size + count) > (BLOCK_SIZE*140)) {
      printk("file_write: exceed max file_size 71680 bytes\n");
      return -1;
   }
   uint8_t* io_buf = sys_malloc(BLOCK_BITMAP);
   if(io_buf == NULL) {
      printk("file_write: sys_malloc for io_buf failed\n");
      return -1;
   }

   // FIXME
   file->fd_pos;
}
int32_t file_read(struct file* file, void* buf, uint32_t count) {
   uint32_t* all_blocks[140];
   uint8_t* io_buf = (uint8_t*)sys_malloc(BLOCK_SIZE);
   uint32_t size = count, size_left = count;
   // FIXME
   return 0;
}
