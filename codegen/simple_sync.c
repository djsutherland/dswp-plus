#include "queue.h"
#include "simple_sync.h"

static pthread_t threads[NUM_THREADS];
static queue_t data_queues[NUM_QUEUES];

void sync_produce(unsigned long long elem, int val_id) {
  queue_push(&data_queues[val_id], elem);
}

unsigned long long sync_consume(int val_id) {
  return queue_pop(&data_queues[val_id]);
}

// called by master thread
// sends function pointers to the auxiliary threads
void sync_delegate(int tid, void *(*fp)(void *), void *args[]) {
  pthread_create(&threads[tid], NULL, fp, (void *)args);
}

void sync_join() {
  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
}
