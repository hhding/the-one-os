#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"
#include "bitmap.h"

#define MAX_FILES_OPEN_PER_PROC 64
#define TASK_NAME_LEN 16

typedef int32_t pid_t;

// 定义了一个函数指针
// 函数指针指向一个函数
// 这个函数返回 void，其参数是一个指针
// 譬如下面的代码：
// void mytest(char* name) { return; }
// void create_thread(thread_func func) {
//      func("myname");
// }
//
// create_thread(mytest);
//
typedef void thread_func(void*);

enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

struct intr_stack {
    uint32_t vec_no;     // 中断号由中断处理程序压入
    uint32_t edi;        // edi -> eax 由中断处理程序 pushad 压入
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;  // popad 不处理 esp
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;       
    uint32_t gs;         // gs -> ds 由中断处理程序逐个压入
    uint32_t fs;
    uint32_t es;
    uint32_t ds;        

/* 以下由cpu从低特权级进入高特权级时压入 */
    uint32_t err_code;       // err_code 由中断处理程序压入
    void (*eip) (void);      // eip -> ss 是 CPU 自动压入的
    uint32_t cs;             // 参考 iretd 指令
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};

/***********  线程栈thread_stack  ***********
 * 线程自己的栈,用于存储线程中待执行的函数
 * 此结构在线程自己的内核栈中位置不固定,
 * 用在switch_to时保存线程环境。
 * 实际位置取决于实际运行情况。
 ******************************************/
struct thread_stack {
   uint32_t ebp;
   uint32_t ebx;
   uint32_t edi;
   uint32_t esi;

/* 线程第一次执行时,eip指向待调用的函数kernel_thread
其它时候,eip是指向switch_to的返回地址*/
   void (*eip) (thread_func* func, void* func_arg);

/*****   以下仅供第一次被调度上cpu时使用   ****/

/* 参数unused_ret只为占位置充数为返回地址 */
   void (*unused_retaddr);
   thread_func* function;   // 由Kernel_thread所调用的函数名
   void* func_arg;    // 由Kernel_thread所调用的函数所需的参数
};

/* 进程或线程的pcb,程序控制块 */
struct task_struct {
   uint32_t* self_kstack;    // 各内核线程都用自己的内核栈
   enum task_status status;
   char name[16];
   uint8_t priority;         // 线程优先级
   uint8_t ticks;           // 每次在 CPU 上执行的 ticks 数
   uint32_t elapsed_ticks;  // 总共 ticks 数
   struct list_elem general_tag;    // 丢到某个队列里面的时候用，表示某状态
   struct list_elem all_list_tag;   // 所有进程清单，销毁了进程就没了
   uint32_t *pgdir;         // 页表虚拟地址
   struct virtual_addr userprog_vaddr;  //用户进程的虚拟地址
   struct mem_block_desc u_block_desc[DESC_CNT];
   int32_t fd_table[MAX_FILES_OPEN_PER_PROC];
   uint32_t cwd_inode_nr;
   pid_t pid;
   pid_t parent_pid;
   uint32_t exit_status;
   uint32_t stack_magic;     // 用这串数字做栈的边界标记,用于检测栈的溢出
};

extern struct list thread_ready_list;
extern struct list thread_all_list;

void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
void init_thread(struct task_struct* pthread, char* name, int prio);
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_args);
struct task_struct* running_thread();
void schedule(void);
void thread_init(void);
void thread_block(enum task_status status);
void thread_unblock(struct task_struct* pthread);
void thread_yield(void);
uint32_t sys_getpid(void);
void sys_ps(void);
pid_t fork_pid(void);
void thread_exit(struct task_struct* thread, bool need_schedule);
#endif

