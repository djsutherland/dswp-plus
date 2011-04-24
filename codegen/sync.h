/*
 * Thin wrappers using a statically allocated array of fixed-length queues.
 */
#ifndef SYNC_H
#define SYNC_H

#define NUM_THREADS 2
#define NUM_QUEUES 256

typedef unsigned int value_id_t;

void sync_init();
void sync_produce(void *elem, value_id_t vid);
void *sync_consume(value_id_t vid);

#endif
