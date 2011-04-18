#include <pthread.h>
#include "queue.h"
#include "sync.h"

pthread_t threads[NUM_THREADS];
queue_t queues[NUM_THREADS];

static void *jump_to_func(void *arg) {
  thread_id_t tid = (thread_id_t)arg;
  while (true) {
    void *fptr = queue_pop(&queues[tid]);
    ((void(*)(void))fptr)();
  }
  return NULL;
}

void sync_init() {
  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    queue_init(&queues[i]);
    pthread_create(&threads[i], NULL, jump_to_func, (void *)i);
  }
}

void sync_produce(thread_id_t tid, void *elem) {
  queue_push(&queues[tid], elem);
}

void *sync_consume(thread_id_t tid) {
  return queue_pop(&queues[tid]);
}
