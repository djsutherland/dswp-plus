#include "queue.h"
#include "sync.h"

queue_t queues[NUM_THREADS];

void init() {
  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    queue_init(&queues[i]);
  }
}

void produce(thread_id_t tid, void *elem) {
  queue_push(&queues[tid], elem);
}

void *consume(thread_id_t tid) {
  return queue_pop(&queues[tid]);
}

void destroy() {
  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    queue_destroy(&queues[i]);
  }
}
