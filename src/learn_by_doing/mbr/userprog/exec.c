#include "exec.h"
#include "string.h"
#include "thread.h"
#include "fs.h"
#include "memory.h"

extern void intr_exit(void);

struct Elf32_Ehdr {
    unsigned char e_ident[16];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    uint32_t    e_entry;
    uint32_t    e_phoff;
    uint32_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
};

struct Elf32_Phdr {
    uint32_t    p_type; // 见下面的 segment_type
    uint32_t    p_offset;
    uint32_t    p_vaddr;
    uint32_t    p_paddr;
    uint32_t    p_filesz;
    uint32_t    p_memsz;
    uint32_t    p_flags;
    uint32_t    p_align;
};

enum segment_type {
    PT_NULL,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR,
};

static bool segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr) {
    uint32_t page_cnt = DIV_ROUND_UP((vaddr & 0x00000fff) + filesz, PAGE_SIZE);
    printk("segment_load: for %x, cnt: %d\n", vaddr, page_cnt);
    uint32_t vaddr_page = vaddr & 0xfffff000;
    for(uint32_t page_idx = 0; page_idx < page_cnt; page_idx++) {
        uint32_t *pde = pde_ptr(vaddr_page);
        uint32_t *pte = pte_ptr(vaddr_page);
        if(!(*pde & 0x00000001) || !(*pte & 0x00000001)) {
            //printk("allocate page for %x, idx: %d\n", vaddr_page, page_idx);
            if(get_a_page(PF_USER, vaddr_page, 1) == NULL) return false;
        }
        vaddr_page += PAGE_SIZE;
    }
    sys_lseek(fd, offset, SEEK_SET);
    sys_read(fd, (void*)vaddr, filesz);
    return true;
}

int32_t load(const char* path) {
    struct Elf32_Ehdr elf_header;
    struct Elf32_Phdr prog_header;
    int32_t ret = -1;
    int32_t fd = sys_open(path, O_RDONLY);
    if(fd == -1) return -1;

    if(sys_read(fd, &elf_header, sizeof(struct Elf32_Ehdr)) != sizeof(struct Elf32_Ehdr)) {
        goto load_done;
    }
    if(memcmp(elf_header.e_ident, "\177ELF\1\1\1", 7) \
    || elf_header.e_type != 2 \
    || elf_header.e_machine != 3 \
    || elf_header.e_version != 1 \
    || elf_header.e_phnum > 1024 \
    || elf_header.e_phentsize != sizeof(struct Elf32_Phdr)) {
        goto load_done;
    }
    uint32_t prog_header_offset = elf_header.e_phoff;
    uint16_t prog_header_size = elf_header.e_phentsize;

    for(uint32_t prog_idx=0; prog_idx < elf_header.e_phnum; 
        prog_idx++, prog_header_offset += elf_header.e_phentsize) {
        printk("load %d of %d segment..\n", prog_idx, elf_header.e_phnum);
        printk("load: seek to %d\n", prog_header_offset);
        sys_lseek(fd, prog_header_offset, SEEK_SET);
        printk("load: read prog header size: %d\n", prog_header_size);
        if(sys_read(fd, &prog_header, prog_header_size) != prog_header_size) {
            printk("load: err: read size not equal to prog_header_size\n");
            goto load_done;
        }
        printk("load: check prog type\n");
        if(PT_LOAD == prog_header.p_type) {
            // printk("load elf: %s vaddr:%x offset:%x size: %x\n", path, prog_header.p_vaddr, prog_header.p_offset, prog_header.p_filesz);
            if(!segment_load(fd, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr)) {
                printk("segment_load failed\n");
                goto load_done;
            }
            // printk("segment_load success\n");
        } else {
            printk("skip unknown prog type: %d\n", prog_header.p_type);
        }
    }
    ret = elf_header.e_entry;
    // printk("load %s entry: %x\n", path, ret);
load_done:
    sys_close(fd);
    return ret;
}

int32_t sys_execv(const char* path, const char* argv[]) {
    uint32_t argc = 0;
    while(argv[++argc]);

    int32_t entry_point = load(path);
    if(entry_point == -1) return -1;

    struct task_struct* cur = running_thread();
    block_desc_init(cur->u_block_desc);
    memcpy(cur->name, path, TASK_NAME_LEN);
    struct intr_stack* intr_0_stack = (struct intr_stack*)((uint32_t)cur + PAGE_SIZE - sizeof(struct intr_stack));
    intr_0_stack->ebx = (int32_t)argv;
    intr_0_stack->ecx = argc;
    intr_0_stack->eip = (void*)entry_point;
    // 下面这行其实没啥用，以 movl 这行为准
    intr_0_stack->esp = (void*)0xc0000000;
    asm volatile("movl %0, %%esp; jmp intr_exit":: "g" (intr_0_stack): "memory");
    return 0;
}