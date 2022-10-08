#include "wait_exit.h"
#include "debug.h"
#include "fs.h"
#include "pipe.h"
#include "memory.h"

static bool find_hanging_child(struct list_elem* pelem, pid_t parent_pid) {
    struct task_struct * pthread = elem2entry(struct task_struct, all_list_tag, pelem);
    if(pthread->parent_pid == parent_pid && pthread->status == TASK_HANGING) return true;
    return false;
}

static bool find_child(struct list_elem* pelem, pid_t parent_pid) {
    struct task_struct * pthread = elem2entry(struct task_struct, all_list_tag, pelem);
    if(pthread->parent_pid == parent_pid) return true;
    return false;
}

pid_t sys_wait(int32_t* status) {
    struct task_struct* cur = running_thread();
    while(1) {
        struct list_elem* child_elem = list_traversal(&thread_all_list, find_hanging_child, cur->pid);
        if(child_elem != NULL) {
            struct task_struct* child_thread = elem2entry(struct task_struct, all_list_tag, child_elem);
            *status = child_thread->exit_status;
            uint16_t child_pid = child_thread->pid;
            // 清理和释放所有资源；传入 false，不调度，返回到此继续
            thread_exit(child_thread, false);
            return child_pid;
        }
        // 这个做法有点丑陋；对于没有子进程瞎等的情况进行处理
        child_elem = list_traversal(&thread_all_list, find_child, cur->pid);
        if(child_elem == NULL) {
            return -1;
        } else {
            thread_block(TASK_WAITING);
        }
    }
}

void release_prog_resource(struct task_struct* pthread) {
    // 页目录就在这里
    uint32_t* pgdir_vaddr = pthread->pgdir;
    // printk("free pgdir: 0x%x\n", (uint32_t)pgdir_vaddr);
    // lower 3GB for user space, 1024 * 3 / 4 = 768
    for(uint32_t pde_idx = 0; pde_idx < 768; pde_idx++) {
        uint32_t pde = *(pgdir_vaddr + pde_idx);
        if(pde & 0x00000001) {
            // printk("free pde_idx: %d 0x%x\n", pde_idx, pde);
            // 这里其实就是虚拟地址了，pde_idx * 0x400000，后者是 4MB
            uint32_t* pte_vaddr = pte_ptr(pde_idx * 0x400000);
            for(uint32_t pte_idx = 0; pte_idx < 1024; pte_idx++) {
                uint32_t pte = *(pte_vaddr + pte_idx);
                if(pte & 0x00000001) {
                    //printk(" %x", pte & 0xfffff000);
                    free_a_phy_page(pte & 0xfffff000);
                }
            }
            //printk("\nfree pde_idx self: %d\n", pde_idx);
            free_a_phy_page(pde & 0xfffff000);
        }
    }
    // 释放 bitmap 所在内存
    uint32_t bitmap_pg_cnt = pthread->userprog_vaddr.vaddr_bitmap.btmap_bytes_len / PAGE_SIZE;
    uint8_t* user_vaddr_pool_bitmap = pthread->userprog_vaddr.vaddr_bitmap.bits;
    mfree_page(PF_KERNEL, user_vaddr_pool_bitmap, bitmap_pg_cnt);

    // 关闭进程打开的文件
    for(uint8_t local_fd = 3; local_fd < MAX_FILES_OPEN_PER_PROC; local_fd++) {
        if(pthread->fd_table[local_fd] != -1) {
            if(is_pipe(local_fd)) {
                // FIXME
            } else {
                sys_close(local_fd);
            }
        }
    }
}

static bool init_adopt_a_child(struct list_elem* pelem, pid_t ppid) {
    struct task_struct * pthread = elem2entry(struct task_struct, all_list_tag, pelem);
    if(pthread->parent_pid == ppid) {
        pthread->parent_pid = 1;
    }
    return false;
}

static bool pid_check(struct list_elem* pelem, pid_t pid) {
    struct task_struct * pthread = elem2entry(struct task_struct, all_list_tag, pelem);
    if(pthread->pid == pid) return true;
    return false;
}

struct task_struct* pid2thread(pid_t ppid) {
    struct list_elem* pelem = list_traversal(&thread_all_list, pid_check, ppid);
    if(pelem == NULL) return NULL;
    return elem2entry(struct task_struct, all_list_tag, pelem);
}

void sys_exit(int32_t status) {
    struct task_struct* cur = running_thread();
    cur->exit_status = status;
    if(cur->parent_pid == -1) PANIC("sys_exit: ppid is -1\n");
    // 自己的子进程给 init
    list_traversal(&thread_all_list, init_adopt_a_child, cur->pid);
    release_prog_resource(cur);

    struct task_struct* parent_thread = pid2thread(cur->parent_pid);
    if(parent_thread->status == TASK_WAITING) {
        thread_unblock(parent_thread);
    }
    thread_block(TASK_HANGING);
}
