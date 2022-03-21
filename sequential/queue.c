#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

queue* queue_construct(uint32_t size) {
    queue* q = calloc(1, sizeof(queue));
    if(!q) 
        return NULL;
    q->size = size;
    q->data = calloc(size, sizeof(void*));
    if(!q->data)
        return NULL;
    return q;
}

void queue_destruct(queue* q) {
    free(q->data);
    free(q);
}

int queue_empty(queue* q) {
    return q->back == q->front;
}

static inline int queue_full(queue* q) {
    return q->back + 1 == q->front || (q->back == q->size - 1 && q->front == 0);
}

void queue_expand(queue* q) {
    void** new_data = calloc(q->size*2, sizeof(void*));
    memcpy(new_data, q->data + q->front, (q->size - q->front)*sizeof(void*));
    if(q->front > q->back){
        memcpy(new_data + q->size - q->front, q->data, q->back*sizeof(void*));
    }
    q->front = 0;
    q->back = q->size - 1;
    q->size *= 2;
    free(q->data);
    q->data = new_data;
}

int queue_push(queue* q, void* d) {
    if(queue_full(q)){
        queue_expand(q);
    }
    q->data[q->back++] = d;
    if(q->back == q->size) 
        q->back = 0;
    return 0;
}

void* queue_front(queue* q) {
    if(queue_empty(q)){
        return NULL;
    }
    return q->data[q->front];
}

void* queue_pop(queue* q) {
    if(queue_empty(q)){
        return NULL;
    }
    void* ret = q->data[q->front++];
    if(q->front == q->size) 
        q->front = 0;
    return ret;
};
