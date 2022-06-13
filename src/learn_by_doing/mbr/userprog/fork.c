#include "fork.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "process.h"

static void copy_pcb(struct task_struct* child_thread, struct task_struct* parent_thread) {
    // 拷贝进程控制快
    memcpy(child_thread, parent_thread, PAGE_SIZE);
    child_thread->pid = fork_pid();
    child_thread->elapsed_ticks = 0;
    child_thread->status = TASK_READY;
    child_thread->ticks = child_thread->priority;
    child_thread->parent_pid = parent_thread->pid;
    child_thread->general_tag.prev = child_thread->general_tag.next = NULL;
    child_thread->all_list_tag.prev = child_thread->all_list_tag.next = NULL;
    block_desc_init(child_thread->u_block_desc);
}

static int32_t copy_vaddrbitmap(struct task_struct* child_thread, struct task_struct* parent_thread) {
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP(
        (0xc00000000 - USER_VADDR_START) / PAGE_SIZE / 8, PAGE_SIZE);
    void* vaddr_btmp = get_kernel_page(bitmap_pg_cnt);
    if(vaddr_btmp == NULL) return -1;
    memcpy(vaddr_btmp, child_thread->userprog_vaddr.bits, bitmap_pg_cnt * PAGE_SIZE);
    child_thread->userprog_vaddr.bits = vaddr_btmp;
    return 0;
}

int32_t copy_process(struct task_struct* child_thread, struct task_struct* parent_thread) {
    // 拷贝进程相关信息
    // 用户态：1. 进程代码，2. 进程数据，3. 进程栈
    // 内核态：
    //  1. pcb（task_struct 结构体）
    //  2. 其他的结构体内的指针，外部分配的内容
    //     * general_tag (struct list_element)，需要更新其指针
    //     * all_list_tag (struct list_element)，需要更新其指针
    //     * struct mem_block_desc u_block_desc[DESC_CNT]，结构体里面有 list
    //     * pgdir，页目录毫无疑问需要更新
    //     * userprog_vaddr.vaddr_bitmap.bits
    //     * self_kstack，这个当时是分配了一个页的
     
    copy_pcb(child_thread, parent_thread);
    child_thread->pgdir = create_page_dir();
    copy_vaddrbitmap(child_thread, parent_thread);

    copy_body_stack3(child_thread, parent_thread);
    build_child_stack(child_thread);
    update_inode_open_cnts(child_thread);
    return 0;
}

/* fork子进程,只能由用户进程通过系统调用fork调用,
   内核线程不可直接调用,原因是要从0级栈中获得esp3等 */
pid_t sys_fork(void) {
    struct task_struct* parent_thread = running_thread();
    ASSERT(parent_thread->pgdir != NULL);
    struct task_struct* child_thread = get_kernel_page(1);
    if(child_thread == NULL) {
        return -1;
    }
    ASSERT(INTR_OFF == intr_get_status());
    if (copy_process(child_thread, parent_thread) == -1) {
        return -1;
    }
    ASSERT(!elem_find(&thread_ready_list, &child_thread->general_tag));
    list_append(&thread_ready_list, &child_thread->general_tag);
    ASSERT(!elem_find(&thread_all_list, &child_thread->all_list_tag));
    list_append(&thread_all_list, &child_thread->all_list_tag);
    return child_thread->pid;
}
