#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

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
    outb (PIC_M_DATA, 0xfe);
    outb (PIC_S_DATA, 0xff);
}

void idt_init() {
    idt_desc_init();
    pic_init();

    uint64_t idt_operand = ((sizeof(idt) -1) | ((uint64_t) ((uint32_t)idt << 16)));
    asm volatile("lidt %0": : "m" (idt_operand));
}
