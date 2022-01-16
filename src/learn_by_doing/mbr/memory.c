#include "memory.h"
#include "printk.h"

#define PAGE_SIZE 4096

#define MEMORY_BITMAP_BASE 0xc009a000
#define K_HEEP_START 0xc0100000 

struct pool {
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};

struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;

static void mem_pool_init(uint32_t all_mem) {
    printk("    mem_pool_init start\n");
    uint32_t used_mem = 256 * PAGE_SIZE + 0x100000; // 页表和低端1MB内存
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PAGE_SIZE;
    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    kernel_pool.phy_addr_start = used_mem;
    kernel_pool.pool_size = kernel_free_pages * PAGE_SIZE;
    kernel_pool.pool_bitmap.bits = (void*)MEMORY_BITMAP_BASE;

    user_pool.phy_addr_start = used_mem + kernel_free_pages * PAGE_SIZE;
    user_pool.pool_size = user_free_pages * PAGE_SIZE;
    user_pool.pool_bitmap.bits = (void*)(MEMORY_BITMAP_BASE + kernel_free_pages / 8);
}

void mem_init() {
    printk("memory_init start\n");
    uint32_t mem_bytes_total = 32*1024*1024;
    mem_pool_init(mem_bytes_total);
    printk("memory_init done\n");
}
