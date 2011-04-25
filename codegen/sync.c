#include <pthread.h>
#include "queue.h"
#include "sync.h"

pthread_t threads[NUM_THREADS];
queue_t func_queues[NUM_THREADS];
queue_t data_queues[NUM_QUEUES];

static void *jump_to_func(void *arg) {
  value_id_t vid = (value_id_t)arg;
  while (true) {
    void *fptr = queue_pop(&func_queues[vid]);
    ((void(*)(void))fptr)();
    // notify master thread that we're done
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

void send_functions(...) {
  // send function pointers to each of the threads
  // block until everyone notifies you
  // return
}

void sync_produce(void *elem, value_id_t vid) {
  queue_push(&data_queues[vid], elem);
}

void *sync_consume(value_id_t vid) {
  return queue_pop(&queues[vid]);
}
