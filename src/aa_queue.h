#ifndef _AA_QUEUE_H
#define _AA_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define CAS(ptr, oldv, newv) __sync_bool_compare_and_swap(ptr, oldv, newv)

typedef void (* travel_func) (void *);

typedef struct {
	void **bucket;
	uint32_t max;
	uint32_t max_rindex;
	uint32_t rindex;
	uint32_t windex;
} queue;

queue *queue_init(uint32_t max);

int queue_enqueue(queue *q, void *data);

int queue_dequeue(queue *q, void **data);

uint32_t queue_size(queue *q);

void queue_travel(queue *q, travel_func tf);

int queue_destroy(queue *q);

#endif
