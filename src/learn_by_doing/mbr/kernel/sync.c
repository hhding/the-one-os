#include "sync.h"
#include "list.h"
#include "thread.h"
#include "interrupt.h"
#include "debug.h"

void sema_init(struct semaphore* psema, uint8_t value) {
    psema->value = value;
    list_init(&psema->waiters);
}

void sema_down(struct semaphore* psema) {
    enum intr_status old_intr_status = intr_disable();
    while(psema->value == 0) {
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }
    psema->value--;
    intr_set_status(old_intr_status);
}

void sema_up(struct semaphore* psema) {
    enum intr_status old_intr_status = intr_disable();
    ASSERT(psema->value == 0);
    psema->value++;
    if(!list_empty(&psema->waiters)) {
        struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    intr_set_status(old_intr_status);
}

void lock_init(struct lock* plock) {
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1);
}

void lock_acquire(struct lock* plock) {
    if(plock->holder != running_thread()) {
        sema_down(&plock->semaphore);
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    } else {
        plock->holder_repeat_nr++;
    }
}

void lock_release(struct lock* plock) {
    ASSERT(plock->holder == running_thread());
    plock->holder_repeat_nr--;
    if(plock->holder_repeat_nr > 0) return;

    plock->holder = NULL;
    sema_up(&plock->semaphore);
}
