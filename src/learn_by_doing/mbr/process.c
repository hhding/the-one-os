#include "global.h"
#include "thread.h"
#include "printk.h"
#include "memory.h"
#include "debug.h"
#include "process.h"
#include "interrupt.h"
#include "string.h"
#include "tss.h"

void create_user_vaddr_bitmap(struct task_struct* thread) {
    thread->userprog_vaddr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8, PAGE_SIZE);
    thread->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    thread->userprog_vaddr.vaddr_bitmap.btmap_bytes_len = (0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8;
    bitmap_init(&thread->userprog_vaddr.vaddr_bitmap);
}

void start_process(void* filename_) {
    struct task_struct* cur = running_thread();
    // 高地址 intr_stack
    //        thread_stack
    // 低地址  <---- self_kstack
    cur->self_kstack += sizeof(struct thread_stack);
    // 开始伪造中断压栈的现场
    struct intr_stack* stack = (struct intr_stack*)cur->self_kstack;
    stack->edi = stack->esi = stack->ebp = stack->esp_dummy = 0;
    stack->ebx = stack->edx = stack->ecx = stack->eax = 0;
    stack->gs = 0;
    stack->fs = stack->es = stack->ds = SELECTOR_U_DATA;
    stack->eip = (void*) filename_;
    stack->cs = SELECTOR_U_CODE;
    stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    // 这个是分配给用户态进程的栈空间，会由 iretd 赋值过去
    stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PAGE_SIZE);
    stack->ss = SELECTOR_U_DATA;
    // 将以上伪装完毕的中断栈设置为 esp，然后基于这个栈，开始恢复现场。
    asm volatile ("movl %0, %%esp; jmp intr_exit": : "g"(stack) : "memory");
/*
intr_exit:
; 以下是恢复上下文环境
   add esp, 4              ; 跳过中断号
   popad
   pop gs
   pop fs
   pop es
   pop ds
   add esp, 4              ; 跳过error_code
   iretd
*/
}

// 更新页表，更新 tss 中的 esp0
void process_activate(struct task_struct* p_thread) {
    ASSERT(p_thread != NULL);
    page_dir_activate(p_thread);
    if(p_thread->pgdir) {
        update_tss_esp(p_thread);
    }
}
void page_dir_activate(struct task_struct* p_thread) {
    uint32_t phy_addr = 0x100000;
    if (p_thread->pgdir != NULL) 
        phy_addr = addr_v2p((uint32_t)p_thread->pgdir);

    asm volatile ("movl %0, %%cr3" : : "r" (phy_addr) : "memory");
}

uint32_t* create_page_dir(void) {
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    printk("create_page_dir<page_dir_vaddr>: %x\n", (uint32_t)page_dir_vaddr);
    ASSERT(page_dir_vaddr != NULL);
    if (page_dir_vaddr == NULL) {
        PANIC("create_page_dir: get_kernel_page falied!");
    }
    // 拷贝页目录（高1GB地址空间，共256个项，每个项要4个字节，共1024字节）
    // memcpy(dst, src, size)
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 768*4), (uint32_t*)(0xfffff000 + 768 * 4), 1024);
    // 拷过来的页目录，其中最后一项应该改成指向其自己
    uint32_t phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    printk("create_page_dir<phy_addr>: %x\n", (uint32_t)phy_addr);
    page_dir_vaddr[1023] = phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    return page_dir_vaddr;
}

void process_execute(void* filename, char* name) {
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, default_prio);
    // 创建一个 bitmap，记录进程自己的虚拟地址空间用掉了哪些
    create_user_vaddr_bitmap(thread);
    thread_create(thread, start_process, filename);
    // 这里就要创建进程用的页表，包括页目录和页表
    thread->pgdir = create_page_dir();
    printk("pgdir %s: %x\n", name, (uint32_t)thread->pgdir);

    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    intr_set_status(old_status);
}

