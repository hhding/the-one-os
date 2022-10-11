#include "pipe.h"
#include "fs.h"
#include "file.h"
#include "memory.h"
#include "ioqueue.h"

bool is_pipe(uint32_t local_fd) {
    uint32_t global_fd = fd_local2global(local_fd);
    return file_table[global_fd].fd_flag == PIPE_FLAG;
}

int32_t sys_pipe(int32_t pipefd[2]) {
    uint32_t fd_idx = get_free_slot_in_global();
    if(fd_idx == -1) return -1;

    file_table[fd_idx].fd_inode = get_kernel_pages(1);
    if(file_table[fd_idx].fd_inode == NULL) return -1;
    ioqueue_init((struct ioqueue*)file_table[fd_idx].fd_inode);

    file_table[fd_idx].fd_pos = 2;
    file_table[fd_idx].fd_flag = PIPE_FLAG;
    pipefd[0] = pcb_fd_install(fd_idx);
    pipefd[1] = pcb_fd_install(fd_idx);
    
    return 0;
}

uint32_t pipe_read(int32_t fd, void* buf, uint32_t count) {
    struct ioqueue* q = (struct ioqueue*)file_table[fd_local2global(fd)].fd_inode;
    char* p = buf;
    while(count--) {
        *p++ = ioq_getchar(q);
    }
    return count;
}

uint32_t pipe_write(int32_t fd, const void* buf, uint32_t count) {
    struct ioqueue* q = (struct ioqueue*)file_table[fd_local2global(fd)].fd_inode;
    char* p = buf;
    while(count--) {
         ioq_putchar(q, *p++);
    }
    return count;
}

void sys_fd_redirect(uint32_t old_local_fd, uint32_t new_local_fd) {
    struct task_struct * cur = running_thread();
    cur->fd_table[old_local_fd] = new_local_fd < 3? new_local_fd: cur->fd_table[new_local_fd];
}
