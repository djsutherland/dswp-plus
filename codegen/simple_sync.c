#include <pthread.h>
#include <stdio.h>
#include "queue.h"
#include "simple_sync.h"

static pthread_t threads[NUM_THREADS];
static queue_t data_queues[NUM_QUEUES];

void showPlace() {
	printf("hello hell!\n");
}

void showPtr(void *elem) {
	printf("show pointer: %d\n", elem);
}

void showValue(unsigned long long elem) {
	printf("show value: %lld \t %d\n", elem, *((int *) elem));
}

void sync_produce(unsigned long long elem, int val_id) {
	printf("lld\n", elem);
	queue_push(&data_queues[val_id], elem);
}

unsigned long long sync_consume(int val_id) {
	unsigned long long res = queue_pop(&data_queues[val_id]);
	printf("lld\n", res);
	return res;
}

// called by master thread
// sends function pointers to the auxiliary threads
void sync_delegate(int tid, void *(*fp)(void *), unsigned long long args[]) {
	printf("create thread!\n");
	pthread_create(&threads[tid], NULL, fp, (void *) args);
}

void sync_join() {
	printf("join\n");
	int i;
	for (i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
}

void sync_init() {
	printf("init\n");
	int i;
	for (i = 0; i < NUM_QUEUES; i++) {
		queue_init(&data_queues[i]);
	}
}

