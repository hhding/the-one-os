#include "thread.h"
#include "printk.h"
#include "memory.h"
#include "debug.h"
#include "process.h"
#include "interrupt.h"

void process_execute(void* filename, char* name) {
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, default_prio);
    // 创建一个 bitmap，记录进程自己的虚拟地址空间用掉了哪些
    create_user_vaddr_bitmap(thread);
    thread_create(thread, start_process, filename);
    // 这里就要创建进程用的页表，包括页目录和页表
    thread->pgdir = create_page_dir();

    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    intr_set_status(old_status);
}

void create_user_vaddr_bitmap(struct task_struct* thread) {
    thread->userprog_vaddr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8, PAGE_SIZE);
    thread->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    thread->userprog_vaddr.vaddr_bitmap.btmap_bytes_len = (0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8;
    bitmap_init(&thread->userprog_vaddr.vaddr_bitmap);
}

void start_process(void* filename_);
void process_activate(struct task_struct* p_thread) {

}
void page_dir_activate(struct task_struct* p_thread);

uint32_t* create_page_dir(void) {
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    if (page_dir_vaddr == NULL) {
        printk("create_page_dir: get_kernel_page falied!");
        return NULL;
    }
    // 拷贝页目录（高1GB地址空间，共256个项，每个项要4个字节，共1024字节）
    // memcpy(dst, src, size)
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 768*4), (uint32_t*)(0xfffff000 + 768 * 4), 1024);
    // 拷过来的页目录，其中最后一项应该改成指向其自己
    uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    return page_dir_vaddr;
}

