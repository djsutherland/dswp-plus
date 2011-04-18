/*
 * Thin wrappers using a statically allocated array of fixed-length queues.
 */
#ifndef SYNC_H
#define SYNC_H

#define NUM_THREADS 2

typedef unsigned int thread_id_t;

void init();
void destroy();
void produce(thread_id_t tid, void *elem);
void *consume(thread_id_t tid);

#endif
