/*
 * Thin wrappers using a statically allocated array of fixed-length queues.
 */
#ifndef SYNC_H
#define SYNC_H

#define NUM_THREADS 2

typedef unsigned int thread_id_t;

void queue_init();
void queue_produce(thread_id_t tid, void *elem);
void *queue_consume(thread_id_t tid);

#endif
