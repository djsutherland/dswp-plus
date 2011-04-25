#include "simple_sync.h"

pthread_t threads[NUM_THREADS];
queue_t data_queues[NUM_QUEUES];

void sync_produce(void *elem, value_id_t vid) {
  queue_push(&data_queues[vid], elem);
}

void *sync_consume(value_id_t vid) {
  return queue_pop(&queues[vid]);
}

// called by master thread
// sends function pointers to the auxiliary threads
void sync_delegate(int tid, void *(*)(void *) fp, void *args[]) {
  pthread_create(&threads[tid], NULL, fp, (void *)args);
}

void sync_join() {
  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(&threads[i], NULL);
  }
}
