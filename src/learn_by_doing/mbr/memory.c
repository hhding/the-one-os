#include "memory.h"
#include "printk.h"
#include "string.h"
#include "debug.h"

#define PAGE_SIZE 4096

#define MEMORY_BITMAP_BASE 0xc009a000
#define K_HEEP_START 0xc0100000 

// 0xfffc00000 = 10 bit 1 + 22 bit 0
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
// 0x003ff000 = 10 bit 0 + 10 bit 1 + 12 bit 0
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

struct pool {
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};

struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;

static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
    int vaddr_start = 0, bit_idx_start = -1;
    if(pf == PF_KERNEL) { 
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if(bit_idx_start == -1) return NULL;
        while(pg_cnt--) {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + pg_cnt -1, 1);
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PAGE_SIZE;
    } else {
            // TODO
    }
    return (void*)vaddr_start;
}

uint32_t* pte_ptr(uint32_t vaddr) {
    uint32_t* pte = (uint32_t*)(0xffc00000 + \
            ((vaddr & 0xffc00000) >> 10) + \
            PTE_IDX(vaddr) * 4);
    return pte;
}

uint32_t* pde_ptr(uint32_t vaddr) {
    uint32_t* pde = (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
    return pde;
}

static void* pmalloc(struct pool* m_pool) {
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if(bit_idx == -1) return NULL;
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = (m_pool->phy_addr_start + bit_idx * PAGE_SIZE);
    return (void*)page_phyaddr;
}

static void page_table_add(void* _vaddr, void* _page_phyaddr) {
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);
    if (!(*pde & 0x00000001)) {    // 页目录如果不存在，那么就分配一个
        uint32_t pde_phyaddr = (uint32_t)pmalloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        memset((void*)((int)pte & 0xfffff000), 0, PAGE_SIZE);
    }
    ASSERT(!(*pte & 0x00000001)); // 既然是分配，也表项不应该存在
    *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
}

static void mem_pool_init(uint32_t all_mem) {
    printk("    mem_pool_init start\n");
    uint32_t used_mem = 256 * PAGE_SIZE + 0x100000; // 页表和低端1MB内存
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PAGE_SIZE;
    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;
    uint32_t kbm_length = kernel_free_pages / 8;
    uint32_t ubm_length = user_free_pages / 8;
    uint32_t kp_start = used_mem;
    uint32_t up_start = used_mem + kernel_free_pages * PAGE_SIZE;

    kernel_pool.phy_addr_start = kp_start;
    kernel_pool.pool_size = kernel_free_pages * PAGE_SIZE;
    kernel_pool.pool_bitmap.bits = (void*)MEMORY_BITMAP_BASE;
    kernel_pool.pool_bitmap.btmap_bytes_len = kbm_length;

    user_pool.phy_addr_start = up_start;
    user_pool.pool_size = user_free_pages * PAGE_SIZE;
    user_pool.pool_bitmap.bits = (void*)(MEMORY_BITMAP_BASE + kbm_length);
    kernel_pool.pool_bitmap.btmap_bytes_len = ubm_length;

    kernel_vaddr.vaddr_start = K_HEEP_START;
    kernel_vaddr.vaddr_bitmap.btmap_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits =  (void*)(MEMORY_BITMAP_BASE + kbm_length + ubm_length);

    bitmap_init(&user_pool.pool_bitmap);
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    printk("    mem_pool_init done\n");
}

void mem_init() {
    printk("memory_init start\n");
    uint32_t mem_bytes_total = 32*1024*1024;
    mem_pool_init(mem_bytes_total);
    printk("memory_init done\n");
}
