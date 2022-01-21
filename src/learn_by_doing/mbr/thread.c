#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "interrupt.h"
#include "list.h"
#include "debug.h"

#define PAGE_SIZE 4096

static void kernel_thread(thread_func* func, void* func_arg) {
    intr_enable();
    func(func_arg);
}

struct task_struct* main_thread;

struct list thread_ready_list;
struct list thread_all_list;
//static struct list_elem* thread_tag;

extern void switch_to(struct task_struct* cur, struct task_struct* next);

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


