#include "interrupt.h"
#include "global.h"
#include "stdint.h"
#include "io.h"
#include "printk.h"

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

#define IDT_DESC_CNT 0x81

#define EFLAGS_IF 0x00000200
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" : "=g" (EFLAG_VAR))

extern intr_handler intr_entry_table[IDT_DESC_CNT];     // 声明引用定义 interrupt_helper.asm
extern intr_handler syscall_handler[10];     // 声明引用定义 interrupt_helper.asm

char* intr_name[IDT_DESC_CNT];           // 用于保存异常的名字
intr_handler asm_intr21_entry(void);
intr_handler idt_table[IDT_DESC_CNT];        // 定义中断处理程序数组.
intr_handler syscall_table[10];        // 定义中断处理程序数组.

struct gate_desc {
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;

    uint8_t attribute;
    uint16_t func_offset_high_word;
};

// idt 在这个C程序的数据段，具体地址取决于链接和载入
static struct gate_desc idt[IDT_DESC_CNT];

static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t) function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

static void idt_desc_init(void) {
    for (int i = 0; i < IDT_DESC_CNT; i++ ) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    make_idt_desc(&idt[0x80], IDT_DESC_ATTR_DPL3, syscall_handler);
}

static void pic_init(void) {
    // 初始化主片
    outb (PIC_M_CTRL, 0x11);
    outb (PIC_M_DATA, 0x20);
    outb (PIC_M_DATA, 0x04);
    outb (PIC_M_DATA, 0x01);

    // 初始化从片
    outb (PIC_S_CTRL, 0x11);
    outb (PIC_S_DATA, 0x28);
    outb (PIC_S_DATA, 0x02);
    outb (PIC_S_DATA, 0x01);

    // 打开从片 IR0, 就是时钟中断
    outb (PIC_M_DATA, 0xf8);
    // outb (PIC_M_DATA, 0xfc);
    outb (PIC_S_DATA, 0xbf);
    // outb (PIC_M_DATA, 0xfd);
    // outb (PIC_S_DATA, 0xff);
}

enum intr_status intr_enable() {
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
    } else {
        old_status = INTR_OFF;
        asm volatile("sti");
    }
    return old_status;
}


enum intr_status intr_disable() {
    enum intr_status old_status;
    if (INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
        asm volatile("cli" : : : "memory");
    } else {
        old_status = INTR_OFF;
    }
    return old_status;
}

enum intr_status intr_set_status(enum intr_status status) {
    return status & INTR_ON? intr_enable(): intr_disable();
}

enum intr_status intr_get_status() {
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (eflags & EFLAGS_IF) ? INTR_ON: INTR_OFF;
}

static void general_intr_handler(uint8_t vec_nr) {
    if(vec_nr == 0x27 || vec_nr == 0x2f) return;
    update_cursor(0);
    int cursor_pos = 0;
    // 前4行清空
    while(cursor_pos++ < 4*80) {
        printk(" ");
    }

    update_cursor(0);
    printk("!!!!!!!       exception message begin   !!!!!!!\n");
    update_cursor(88);
    printk(intr_name[vec_nr]);
    if(vec_nr == 14) {  // Pagefault
        uint32_t page_fault_vaddr = 0;
        asm ("movl %%cr2, %0" : "=r" (page_fault_vaddr));
        printk("\npage fault addr is 0x%x", page_fault_vaddr);
    }
    printk("\n!!!!!!!       exception message end     !!!!!!!\n");
    while(1);
}

static void exception_init(void) {
    for(int i = 0; i < IDT_DESC_CNT; i++) {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";
    }
   intr_name[0] = "#DE Divide Error";
   intr_name[1] = "#DB Debug Exception";
   intr_name[2] = "NMI Interrupt";
   intr_name[3] = "#BP Breakpoint Exception";
   intr_name[4] = "#OF Overflow Exception";
   intr_name[5] = "#BR BOUND Range Exceeded Exception";
   intr_name[6] = "#UD Invalid Opcode Exception";
   intr_name[7] = "#NM Device Not Available Exception";
   intr_name[8] = "#DF Double Fault Exception";
   intr_name[9] = "Coprocessor Segment Overrun";
   intr_name[10] = "#TS Invalid TSS Exception";
   intr_name[11] = "#NP Segment Not Present";
   intr_name[12] = "#SS Stack Fault Exception";
   intr_name[13] = "#GP General Protection Exception";
   intr_name[14] = "#PF Page-Fault Exception";
   // intr_name[15] 第15项是intel保留项，未使用
   intr_name[16] = "#MF x87 FPU Floating-Point Error";
   intr_name[17] = "#AC Alignment Check Exception";
   intr_name[18] = "#MC Machine-Check Exception";
   intr_name[19] = "#XF SIMD Floating-Point Exception";
   intr_name[0x80] = "Syscall for OS";
}

void idt_init() {
    printk("idt_init start\n");
    idt_desc_init();
    exception_init();
    pic_init();

    uint64_t idt_operand = ((sizeof(idt) -1) | ((uint64_t) ((uint32_t)idt << 16)));
    //printk("idt addr: %x\n", (uint32_t)idt);
    asm volatile("lidt %0": : "m" (idt_operand));
    printk("idt_init done\n");
}

void register_handler(uint8_t vector_no, intr_handler func) {
    idt_table[vector_no] = func;
}

void register_syscall(uint8_t syscall_nr, intr_handler func) {
    syscall_table[syscall_nr] = func;
}

