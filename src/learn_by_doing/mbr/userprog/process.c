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

// 进程主体包括进程调度信息（pcb），进程内核空间的栈，进程用户空间的栈，进程的代码，数据等。还有 tss。
// 1. pcb，这个本身没特别好说的，调度依靠 pcb 来确定进程并且分配相应的资源。
// 2. 所有的调度事件都是由中断触发的，可能是时间中断、网卡中断等等
//    这意味着用户程序在执行时，其实是无法被中断的，必须要等到某个中断触发才行。
// 3. 当中断发生时，CPU 执行中断处理之前，会自动保存当前运行进程的上下文。
//    如果上下文是低级别，那么需要确定高级别的保存地址，这个地址由 tss 的 esp0 来确定。
//    具体的讲，这个 esp0 指向 pcb 中的 intr_stack ((uint32_t)pcb + page_size)
//    之后，cpu 开始执行中断处理程序。
//    然后 cpu 执行 iretd 从中断中返回。
// 4. 对于 CPU 从中断中返回，在本实现中是很重要的。这里以创建一个用户态进程的角度来做个总览。
//    背景，创建一个用户态进程，需要准备用户态进程的代码，堆和栈。
//    iretd 返回到用户态执行代码，至少需要：cs，eip，eflags（代码相关），ss，esp（栈相关）。iretd 会从 intr_stack 恢复以上寄存器。
//      如果是新进程，需要单独分配内存（pf_user)，然后 esp 指向该地址。
//      其他寄存器和选择子也同样处理。
//      问题：esi，edi，ebx，ebp 在 thread_stack 中也有，intr_stack 中也有，是不是重复了？
//      实际上，中断中 CPU 自动压栈的寄存器就只有指令执行，栈和eflags相关的几个。
//    从中断退出的角度来看，如果要跳到新进程，那么对于进入中断时候压的内容，要完整的构造一份。
//    然后将 esp 指向该内容的最低地址，即栈顶，然后执行退出中断的部分代码。后者其实也就是恢复寄存器的内容，然后跳到用户态代码和恢复到用户态栈。
// 5. process_execute 做为一个进程创建函数，其准备了 switch_to 所需要的环境。但是要注意的是，它并没有跳转去执行新的代码。
//      实际上，代码要从哪里继续执行，只跟中断栈中的 eip 有关。跟 switch_to 没有关系。所以最终目标是控制中断的 eip。
//      在下一次中断触发的 switch_to 中，调度到了新进程，其返回地址并不是 schedule，而是 kernel_thread，这是 thread_create 中完成的。
//      而 kernel_thread 会执行 start_process，后者会伪造中断栈，设置 eip 为执行代码地址。然后跳转到中断退出代码。
//      thread_create -> 控制 switch_to 去执行 kernel_thread，参数为 start_process。
//      start_process 伪造中断现场，最终从中断直接跳转到　到用户代码。



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
    stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PAGE_SIZE, 1);
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

    //printk("page_dir_activate: cr3 => 0x%x\n", phy_addr);
    asm volatile ("movl %0, %%cr3" : : "r" (phy_addr) : "memory");
}

uint32_t* create_page_dir(void) {
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    // printk("create_page_dir<page_dir_vaddr>: %x\n", (uint32_t)page_dir_vaddr);
    ASSERT(page_dir_vaddr != NULL);
    if (page_dir_vaddr == NULL) {
        PANIC("create_page_dir: get_kernel_page falied!");
    }
    // 拷贝页目录（高1GB地址空间，共256个项，每个项要4个字节，共1024字节）
    // memcpy(dst, src, size)
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 768*4), (uint32_t*)(0xfffff000 + 768 * 4), 1024);
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr), (uint32_t*)(0xfffff000), 4);
    // 拷过来的页目录，其中最后一项应该改成指向其自己
    uint32_t phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    // printk("create_page_dir<phy_addr>: %x\n", (uint32_t)phy_addr);
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
    block_desc_init(thread->u_block_desc);
    //printk("pgdir %s: %x\n", name, (uint32_t)thread->pgdir);

    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    intr_set_status(old_status);
}

