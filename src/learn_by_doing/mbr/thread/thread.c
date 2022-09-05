#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "interrupt.h"
#include "list.h"
#include "debug.h"
#include "printk.h"
#include "process.h"
#include "stdio.h"
#include "syscall.h"

#define PAGE_SIZE 4096

static void kernel_thread(thread_func* func, void* func_arg) {
    intr_enable();
    func(func_arg);
}

struct task_struct* main_thread;
struct task_struct* idle_thread;

struct list thread_ready_list;
struct list thread_all_list;
static struct list_elem* thread_tag;
static uint32_t pid = 0;

extern void switch_to(struct task_struct* cur, struct task_struct* next);

uint32_t allocate_pid() {
    pid ++;
    return pid;
}

uint32_t fork_pid() { return allocate_pid();}

struct task_struct* running_thread() {
    uint32_t esp;
    // 因为在一个页里面，所以找到栈对齐就好了
    asm ("mov %%esp, %0" : "=g" (esp));
    return (struct task_struct*)(esp & 0xfffff000);
}

void thread_create(struct task_struct* pthread, thread_func func, void* func_arg) {
    pthread->self_kstack -= sizeof(struct intr_stack);
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = func;
    kthread_stack->func_arg = func_arg;
    // printk("thread_create(%s): eip: 0x%x, func: 0x%x, arg: %x\n", pthread->name, (uint32_t)kernel_thread, (uint32_t)func, (uint32_t)func_arg);
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

void init_thread(struct task_struct* pthread, char* name, int prio) {
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->status = TASK_READY;
    pthread->pgdir = NULL;
    pthread->pid = allocate_pid();
    pthread->fd_table[0] = 0;
    pthread->fd_table[1] = 1;
    pthread->fd_table[2] = 2;
    for(int fd_idx=3; fd_idx < MAX_FILES_OPEN_PER_PROC; fd_idx++) {
        pthread->fd_table[fd_idx] = -1;
    }
    pthread->stack_magic = 20220120;
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PAGE_SIZE);
}

struct task_struct* thread_start(char* name, int prio, thread_func func, void* func_arg) {
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, prio);
    thread_create(thread, func, func_arg);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    return thread;
}

static void make_main_thread(void) {
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    main_thread->status = TASK_RUNNING;

    // running 状态的不需要加到 ready list
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void schedule() {
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct * cur = running_thread();
    if(cur->status == TASK_RUNNING) {
        // 从 running 过来的时时间片用完了
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->status = TASK_READY;
        cur->ticks = cur->priority;
    } else {
        // TODO
        // 其他状态处理
    }
    if(list_empty(&thread_ready_list)) {
        thread_unblock(idle_thread);
    }

    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    process_activate(next);
    // printk("schedule: switch to: %s to %s\n", cur->name, next->name);
    switch_to(cur, next);
}

void thread_yield() {
    enum intr_status old_intr_status = intr_disable();
    schedule();
    intr_set_status(old_intr_status);
}

void idle(void* arg) {
    while(1) {
        thread_block(TASK_BLOCKED);
        asm volatile("sti; hlt" : : :"memory");
    }
}

void thread_block(enum task_status status) {
    // 设置当前线程为 blocked 状态，然后调用 schedule 调度其他线程
    //
    ASSERT(status == TASK_WAITING || status == TASK_BLOCKED || status == TASK_HANGING);
    enum intr_status old_intr_status = intr_disable();
    struct task_struct* cur = running_thread();
    ASSERT(cur->status == TASK_RUNNING);
    cur->status = status;
    schedule();
    intr_set_status(old_intr_status);
}

void thread_unblock(struct task_struct* pthread) {
    ASSERT(pthread->status == TASK_WAITING || pthread->status == TASK_BLOCKED || pthread->status == TASK_HANGING);
    enum intr_status old_intr_status = intr_disable();
    if(pthread->status != TASK_READY) { // 这行挺尴尬，对 ready 的 thread 执行 unblock 是何居心？
        if(elem_find(&thread_ready_list, &pthread->general_tag)) PANIC("thread_unblock: blocked thread in ready list");
        list_push(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }
    intr_set_status(old_intr_status);
}

uint32_t sys_getpid(void) {
    struct task_struct * cur = running_thread();
    return cur->pid;
}

void init(void) {
    uint32_t _pid = fork();
    if(_pid) {
        printf("iii I am father:\n");
        printf("I am father: %d\n", _pid);
    } else {
        printf("iii I am child:\n");
        printf("I am child: %d\n", getpid());
    }
    while(1);
}
void thread_init(void) {
    printk("thread_init start\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);

    process_execute(init, "init");
    make_main_thread();
    idle_thread = thread_start("idle", 10, idle, NULL);
    printk("thread_init done\n");
}

