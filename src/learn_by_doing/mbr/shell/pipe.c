#include "pipe.h"

bool is_pipe(uint32_t local_fd) {
    return true;
}

int32_t sys_pipe(int32_t pipefd[2]) {
    return 0;
}

uint32_t pipe_read(int32_t fd, void* buf, uint32_t count);
uint32_t pipe_write(int32_t fd, const void* buf, uint32_t count);
void sys_fd_redirect(uint32_t old_local_fd, uint32_t new_local_fd);
