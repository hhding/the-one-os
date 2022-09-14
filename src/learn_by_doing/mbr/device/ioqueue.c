#include "ioqueue.h"
#include "debug.h"
#include "interrupt.h"
#include "printk.h"

void ioqueue_init(struct ioqueue* ioq) {
  lock_init(&ioq->lock);
  ioq->consumer = ioq->producer = NULL;
  ioq->head = ioq->tail = 0;
}

static void ioq_wait(struct task_struct** waiter) {
  ASSERT(*waiter == NULL);
  *waiter = running_thread();
  thread_block(TASK_BLOCKED);
}

static int32_t next_pos(int32_t pos) {
  return (pos + 1) % bufsize;
}

bool ioq_full(struct ioqueue* ioq) {
  return next_pos(ioq->head) == ioq->tail;
}

bool ioq_empty(struct ioqueue* ioq) {
  return ioq->head == ioq->tail;
}

static void wakeup(struct task_struct** waiter) {
  ASSERT(*waiter != NULL);
  thread_unblock(*waiter);
  *waiter = NULL;
}

char ioq_getchar(struct ioqueue* ioq) {
  ASSERT(intr_get_status() == INTR_OFF);
  if(ioq_empty(ioq)) {
    lock_acquire(&ioq->lock);
    ioq_wait(&ioq->consumer);
    lock_release(&ioq->lock);
  }

  char byte = ioq->buf[ioq->tail];
  ioq->tail = next_pos(ioq->tail);
  return byte;
}

void ioq_putchar(struct ioqueue* ioq, char byte) {
  ASSERT(intr_get_status() == INTR_OFF);
  if(ioq_full(ioq)) {
    printk("keyboard io full..\n");
    return;
  }
  ioq->buf[ioq->head] = byte;
  ioq->head = next_pos(ioq->head);
  if(ioq->consumer != NULL) {
    wakeup(&ioq->consumer);
  }
}

uint32_t ioq_length(struct ioqueue* ioq) {
  int32_t len = ioq->head - ioq->tail;
  return len > 0? len: bufsize - len;
}

