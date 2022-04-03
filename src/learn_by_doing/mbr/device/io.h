#ifndef __LIB_IO_H
#define __LIB_IO_H
#include "stdint.h"

/*
 * asm ( 汇编语句
    : 输出操作数     // 非必需
    : 输入操作数     // 非必需
    : 其他被污染的寄存器 // 非必需
    );
 */

// 向端口 port 写入一个 byte
static inline void outb(uint16_t port, uint8_t data) {
    // 这里 %0, %1 对应后面的参数
    // b表示byte，只取8位，w表示word，这里是16位
    // a 表示用 ax，d 表示 dx
    asm volatile ("outb %b0, %w1" : : "a" (data), "d" (port));
}

// 将 addr 处开始的 word_cnt 个字写入端口 port
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt) {
    // outsw 是把 ds:esi 处的 16 位的内容写入 port 端口(edx中指定), S 表示 esi
    // rep 重复指令，直到 ecx 为0，这里 word_cnt 赋值给 ecx
    // cld 会让 ecx 自动正确的减少数值
    asm volatile ("cld; rep outsw" : "+c" (word_cnt), "+S" (addr) : "d" (port));
}

// 将从端口 port 读入的一个字节返回
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %w1, %b0": "=a" (data) : "Nd" (port));
    return data;
}

// 将从端口 port 读入的 word_cnt 个字写入 addr
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt) {
    asm volatile ("cld; rep insw" : "+c" (word_cnt), "+D" (addr) : "d" (port) : "memory");
}

#endif
