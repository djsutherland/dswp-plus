#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue.h"

static queue_t *q;

static void *produce(void *arg) {
  int i;
  for (i = 0; i < 100000; i++) {
    push(q, (void *)1);
  }
  return NULL;
}

static void *consume(void *arg) {
  int i;
  unsigned int sum = 0;
  for (i = 0; i < 100000; i++) {
    sum += (unsigned int)pop(q);
  }
  return (void *)sum;
}

static void *block_consume(void *arg) {
  unsigned int result = (unsigned int)pop(q);
  return (void *)result;
}

static void *block_produce(void *arg) {
  int i;
  for (i = 0; i < QUEUE_MAXLEN + 1; i++) {
    push(q, (void *)i);
  }
  printf("Blocked producer done!\n");
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t producers[10];
  pthread_t consumers[10];
  int i;

  printf("Initialize queue...");
  init(&q);
  printf("Done.\n");
  
  printf("Creating producers and consumers...");
  for (i = 0; i < 10; i++) {
    pthread_create(&producers[i], NULL, produce, NULL);
    pthread_create(&consumers[i], NULL, consume, NULL);
  }
  printf("Done.\n");

  printf("Checking answers...");
  unsigned int ret_val;
  for (i = 0; i < 10; i++) {
    pthread_join(producers[i], NULL);
    pthread_join(consumers[i], ((void **)&ret_val));
    if (ret_val != 100000) {
      printf("ERROR: values mismatched: %u\n", ret_val);
    }
  }
  printf("Done.\n");

  printf("Checking blocking functionality...\n");
  pthread_create(&consumers[0], NULL, block_consume, NULL);
  sleep(2);
  push(q, (void *)42);
  pthread_join(consumers[0], ((void **)&ret_val));
  if (ret_val != 42) {
    printf("ERROR: value mismatched: %u\n", ret_val);
  }
  
  pthread_create(&producers[0], NULL, block_produce, NULL);
  printf("Sleeping: producer shouldn't finish during this time.\n");
  sleep(2);
  printf("Done sleeping, consuming so producer can finish...\n");
  pop(q);
  pthread_join(producers[0], NULL);
  printf("Done.\n");

  return 0;
}
