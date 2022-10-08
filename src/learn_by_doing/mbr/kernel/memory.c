#include "memory.h"
#include "printk.h"
#include "string.h"
#include "debug.h"
#include "thread.h"
#include "sync.h"
#include "interrupt.h"

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
    struct lock lock;
};

struct mem_block_desc k_block_desc[DESC_CNT];
struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;

void free_a_phy_page(uint32_t pg_phy_addr) {
    struct pool* pool = pg_phy_addr > user_pool.phy_addr_start ? &user_pool: &kernel_pool;
    bitmap_set(&pool->pool_bitmap, (pg_phy_addr - pool->phy_addr_start) / PAGE_SIZE, 0);
}

static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
    struct virtual_addr* vaddr = NULL;
    int vaddr_start = 0, bit_idx_start = -1;
    if(pf == PF_KERNEL) { 
        vaddr = &kernel_vaddr;
    } else {
            struct task_struct* thread = running_thread();
            vaddr = &thread->userprog_vaddr;
    }
    bit_idx_start = bitmap_scan(&vaddr->vaddr_bitmap, pg_cnt);
    if(bit_idx_start == -1) return NULL;
    while(pg_cnt--) {
        bitmap_set(&vaddr->vaddr_bitmap, bit_idx_start + pg_cnt, 1);
    }
    vaddr_start = vaddr->vaddr_start + bit_idx_start * PAGE_SIZE;
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

uint32_t addr_v2p(uint32_t vaddr) {
    uint32_t* pte = pte_ptr(vaddr);
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

static void* palloc(struct pool* m_pool) {
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if(bit_idx == -1) return NULL;
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = (m_pool->phy_addr_start + bit_idx * PAGE_SIZE);
    return (void*)page_phyaddr;
}

static void page_table_add(void* _vaddr, void* _page_phyaddr) {
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    // printk("vaddr: 0x%x phyaddr: 0x%x\n", vaddr, page_phyaddr);
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);
    // printk("pde: 0x%x pte: 0x%x\n", pde, pte);
    if (!(*pde & 0x00000001)) {    // 页目录如果不存在，那么就分配一个
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        // 前面20位即可
        memset((void*)((int)pte & 0xfffff000), 0, PAGE_SIZE);
    }
    ASSERT(!(*pte & 0x00000001)); // 既然是分配，也表项不应该存在
    *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
}

void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
    void* vaddr_start = vaddr_get(pf, pg_cnt);
    if(vaddr_start == NULL) return NULL;
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    while(cnt--) {
        void* page_phyaddr = palloc(mem_pool);
        if(page_phyaddr == NULL) return NULL;
        // printk("Mapping: 0x%x => 0x%x\n", vaddr, (uint32_t)page_phyaddr);
        page_table_add((void*)vaddr, page_phyaddr);
        vaddr += PAGE_SIZE;
    }
    return vaddr_start;
}

void* get_kernel_pages(uint32_t pg_cnt) {
    lock_acquire(&kernel_pool.lock);
    void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if(vaddr != NULL) {
        memset(vaddr, 0, pg_cnt * PAGE_SIZE);
    }
    lock_release(&kernel_pool.lock);
    return vaddr;
}

void* get_user_pages(uint32_t pg_cnt) {
    lock_acquire(&user_pool.lock);
    void* vaddr = malloc_page(PF_USER, pg_cnt);
    if(vaddr != NULL) {
        memset(vaddr, 0, pg_cnt * PAGE_SIZE);
    }
    lock_release(&user_pool.lock);
    return vaddr;
}

int update_thread_bitmap(enum pool_flags pf, uint32_t vaddr) {
    uint32_t idx = -1;
    struct virtual_addr* thread_vaddr = NULL;
    struct task_struct* cur = running_thread();

    if(cur->pgdir != NULL && pf == PF_USER) {
        thread_vaddr = &cur->userprog_vaddr;
    } else if(cur->pgdir == NULL && pf == PF_KERNEL) {
        thread_vaddr = &kernel_vaddr;
    } else {
        PANIC("get_a_page:not allow kernel alloc userspace or user alloc kernelspace");
    }

    idx = (vaddr - thread_vaddr->vaddr_start) / PAGE_SIZE;
    ASSERT(idx > 0);

    bitmap_set(&thread_vaddr->vaddr_bitmap, idx, 1);
    return 0;
}

void* get_a_page(enum pool_flags pf, uint32_t vaddr, uint32_t op_bitmap) {
    struct pool* mem_pool = (pf & PF_KERNEL ? &kernel_pool : &user_pool);
    lock_acquire(&mem_pool->lock);

    void* page_phyaddr = palloc(mem_pool);
    if(page_phyaddr == NULL) goto get_a_page_fail;

    if(op_bitmap) {
        update_thread_bitmap(pf, vaddr);
    }
    page_table_add((void*)vaddr, page_phyaddr);
    lock_release(&mem_pool->lock);

    return (void*)vaddr;

get_a_page_fail:
    lock_release(&mem_pool->lock);
    PANIC("get_a_page fail");
    return NULL;

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
    printk("        kernel: phy_addr_start: 0x%x pool_size: %d kbytes\n", kp_start, kernel_pool.pool_size/1024);

    user_pool.phy_addr_start = up_start;
    user_pool.pool_size = user_free_pages * PAGE_SIZE;
    user_pool.pool_bitmap.bits = (void*)(MEMORY_BITMAP_BASE + kbm_length);
    user_pool.pool_bitmap.btmap_bytes_len = ubm_length;
    printk("        user: phy_addr_start: 0x%x pool_size: %d kbytes\n", up_start, user_pool.pool_size/1024);

    // user vaddr 各在各自的 pgdir 维护，所以这里没有这部分代码
    // 初始化 kernel vaddr 及其 bitmap
    kernel_vaddr.vaddr_start = K_HEEP_START;
    kernel_vaddr.vaddr_bitmap.btmap_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits =  (void*)(MEMORY_BITMAP_BASE + kbm_length + ubm_length);
    bitmap_init(&user_pool.pool_bitmap);
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&kernel_vaddr.vaddr_bitmap);

    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);
    printk("    mem_pool_init done\n");
}

struct mem_block* arena2block(struct arena* a, uint32_t idx) {
    return (struct mem_block*)((uint32_t)(a+1) + a->desc->block_size * idx);
}

struct arena* block2arena(struct mem_block* b) {
    return (struct arena*)((uint32_t)b & 0xFFFFF000);
}

void* sys_malloc(uint32_t size) {
    //printk("sys_malloc: size: %d\n", size);
    struct task_struct* cur = running_thread();
    //printk("sys_malloc: small size: %d\n", size);
    struct pool* mem_pool;
    enum pool_flags PF;
    struct mem_block_desc* desc;

    if(cur->pgdir == NULL) {
        mem_pool = &kernel_pool;
        PF = PF_KERNEL;
        desc = k_block_desc;
    } else {
        mem_pool = &user_pool;
        PF = PF_USER;
        desc = cur->u_block_desc;
    }

    if( !(size > 0 && size < mem_pool->pool_size) ) return NULL;

    struct arena* a;
    struct mem_block* b;
    lock_acquire(&mem_pool->lock);
    if(size > 1024) {
        uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PAGE_SIZE);
        a = malloc_page(PF, page_cnt);
        if(a!=NULL) {
            memset(a, 0, page_cnt * PAGE_SIZE);
            a->desc = NULL;
            a->cnt = page_cnt;
            a->large = true;
            lock_release(&mem_pool->lock);
            return (void*)(a+1);    // 这里实际上跨过了一个 arena 的长度
        } else {
            lock_release(&mem_pool->lock);
            return NULL;
        }
    } else {
        // 先找到应该用哪个大小的内存块
        int a_idx;
        for(a_idx = 0; a_idx < DESC_CNT; a_idx++) {
            if( size <= desc[a_idx].block_size ) break;
        }
        //printk("sys_malloc: got a_idx %d\n", a_idx);
        // 分配内存块
        // 先看一下有没有空闲的
        if(list_empty(&desc[a_idx].free_list)) {
            a = malloc_page(PF, 1);
            if(a == NULL) {
                lock_release(&mem_pool->lock);
                return NULL;
            }
            memset(a, 0, PAGE_SIZE);
            a->desc = &desc[a_idx];
            a->cnt = desc[a_idx].block_per_arena;
            a->large = false;
            uint32_t b_idx;
            enum intr_status old_status = intr_disable();
            for(b_idx = 0; b_idx < desc[a_idx].block_per_arena; b_idx++) {
                b = arena2block(a, b_idx);
                ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
                list_append(&a->desc->free_list, &b->free_elem);
            }
            intr_set_status(old_status);
        }
        struct list_elem * l = list_pop(&(desc[a_idx].free_list));
        b = elem2entry(struct mem_block, free_elem, l);
        a = block2arena(b);
        a->cnt--;
        lock_release(&mem_pool->lock);
        // printk("malloc size: %d %x\n", size, (uint32_t)b);
        return (void*)b;
    }
}

// 设置bitmap，标记物理内存没在用
// user_pool 在高位
void pfree(uint32_t paddr) {
    uint32_t idx;
    struct pool* mem_pool;
    if(paddr > user_pool.phy_addr_start)
        mem_pool = &user_pool;
    else
        mem_pool = &kernel_pool;
    idx = ((uint32_t)paddr - mem_pool->phy_addr_start) / PAGE_SIZE;
    bitmap_set(&mem_pool->pool_bitmap, idx, 0);
}

static void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t cnt) {
    uint32_t vaddr = (uint32_t)_vaddr;
    struct virtual_addr* vaddr_ptr;
    if(pf == PF_KERNEL)
        vaddr_ptr = &kernel_vaddr;
    else {
        struct task_struct* cur = running_thread();
        vaddr_ptr = &cur->userprog_vaddr;
    }
    uint32_t idx_start = (vaddr - vaddr_ptr->vaddr_start) / PAGE_SIZE;
    for(uint32_t i=0; i< cnt; i++) {
        bitmap_set(&vaddr_ptr->vaddr_bitmap, idx_start + i, 0);
    }
}

void page_table_pte_remove(uint32_t vaddr) {
    uint32_t* pte = pte_ptr(vaddr);
    *pte &= ~PG_P_1;
    asm volatile ("invlpg %0"::"m" (vaddr): "memory");
}

void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t cnt) {
    uint32_t pg_phy_addr;
    uint32_t vaddr = (uint32_t)_vaddr;
    ASSERT(cnt >= 1 && vaddr % PAGE_SIZE == 0);
    pg_phy_addr = addr_v2p(vaddr);
    ASSERT((pg_phy_addr % PAGE_SIZE) == 0 && pg_phy_addr >= 0x102000);

    for(uint32_t i=0; i < cnt; i++) {
        pg_phy_addr = addr_v2p(vaddr);
        pfree(pg_phy_addr);
        page_table_pte_remove(vaddr);
        vaddr += PAGE_SIZE;
    }
    vaddr_remove(pf, _vaddr, cnt);
}

void sys_free(void* ptr) {
    struct task_struct* cur = running_thread();
    enum pool_flags pf;
    struct pool* mem_pool;
    if(cur->pgdir == NULL) {
        pf = PF_KERNEL;
        mem_pool = &kernel_pool;
    } else {
        pf = PF_USER;
        mem_pool = &user_pool;
    }

    lock_acquire(&mem_pool->lock);
    struct mem_block* b = ptr;
    struct arena* a = block2arena(b);
    ASSERT(a->large == 0 || a->large == 1);
    if(a->large == true && a->desc == NULL) {
        mfree_page(pf, a, a->cnt);
    } else {
        list_append(&a->desc->free_list, &b->free_elem);
        a->cnt++;
        /*
        sys_write(1, "sys_free: block_per_arena: ");
        sys_putchar(a->desc->block_per_arena + 48);
        sys_write(1, " cnt: ");
        sys_putchar(a->cnt + 48);
        sys_write(1, "\n");
        */
        if(a->cnt == a->desc->block_per_arena) {
            for(uint32_t i = 0; i < a->desc->block_per_arena; i++) {
                b = arena2block(a, i);
                ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
                list_remove(&b->free_elem);
            }
            mfree_page(pf, a, 1);
        }
    }
    lock_release(&mem_pool->lock);
}

void block_desc_init(struct mem_block_desc* desc_arry) {
    // 16, 32, 64, 128, 256, 512, 1024.
    int block_size = 16;
    for(int i=0; i < DESC_CNT; i++) {
        desc_arry[i].block_size = block_size;
        desc_arry[i].block_per_arena = (PAGE_SIZE - sizeof(struct arena)) / block_size;
        list_init(&desc_arry[i].free_list);
        block_size *= 2;
    }
}

void mem_init() {
    printk("memory_init start\n");
    uint32_t mem_bytes_total = 32*1024*1024;
    mem_pool_init(mem_bytes_total);
    block_desc_init(k_block_desc);
    uint32_t* pde = pde_ptr(0);
    // dirty fix, release the first 1 MB from virtual addr in kernel space.
    printk("pde: %x pde[0]: %x *pde: %x\n", pde[1023], pde[0], *pde);
    pde[0] = 0;
    asm volatile ("movl %0, %%cr3" : : "r" (pde[1023]) : "memory");
    printk("memory_init done\n");
}
