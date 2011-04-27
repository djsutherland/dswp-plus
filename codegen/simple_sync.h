#ifndef SIMPLE_SYNC_H
#define SIMPLE_SYNC_H

#define NUM_THREADS 2
#define NUM_QUEUES 256

void sync_produce(void *elem, int val_id);
void *sync_consume(int val_id);
void sync_delegate(int tid, void void (*fp)(void *), void *args[]);
void sync_join();

#endif
