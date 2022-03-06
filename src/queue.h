#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

/*
    front must have data except when empty
    back must be empty.
*/
typedef struct queue {
    uint32_t size, front, back;
    void **data;
} queue;

queue* queue_construct(uint32_t size);
void queue_destruct(queue* q);
int queue_empty(queue* q);
int queue_push(queue* q, void* d);
void* queue_front(queue* q);
void* queue_pop(queue* q);

#endif