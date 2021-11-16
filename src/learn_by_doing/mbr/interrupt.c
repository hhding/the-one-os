#include "interrupt.h"
#include "global.h"
#include "stdint.h"
#include "io.h"
#include "printk.h"

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

#define IDT_DESC_CNT 0x22

intr_handler asm_intr21_entry(void);

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
    int i = 0;
    for ( i = 0; i < IDT_DESC_CNT; i++ ) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, 0);
    }
    make_idt_desc(&idt[0x21], IDT_DESC_ATTR_DPL0, asm_intr21_entry);
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
    //outb (PIC_M_DATA, 0xfe);
    //outb (PIC_S_DATA, 0xff);
    outb (PIC_M_DATA, 0xfd);
    outb (PIC_S_DATA, 0xff);
}

void intr_handler21(void) {
    int data;
    data = inb(0x60);
    printk("got kb 0x%x\n", data);
    return;
}

void idt_init() {
    idt_desc_init();
    pic_init();

    uint64_t idt_operand = ((sizeof(idt) -1) | ((uint64_t) ((uint32_t)idt << 16)));
    asm volatile("lidt %0": : "m" (idt_operand));
}

