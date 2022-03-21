#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

/*
    front must have data except when empty.
    back must be empty.
*/
typedef struct queue {
    uint32_t size, front, back;
    void **data;
} queue;

/**
 * @brief Construct a queue
 * 
 * @param size the initial size of the queue.
 * @return queue* pointing to the constructed queue, could be NULL if failed.
 */
queue* queue_construct(uint32_t size);

/**
 * @brief Destruct a queue
 * 
 * @param q the queue to be destructed.
 */
void queue_destruct(queue* q);

/**
 * @brief Check if a queue is empty
 * 
 * @param q the queue to check
 * @return non-zero if empty, 0 otherwise
 */
int queue_empty(queue* q);

/**
 * @brief Push an element into the queue.
 * Will Automatically expand if full.
 * @param q the queue to push into
 * @param d the data, note that it is void*.
 * @return int 
 */
int queue_push(queue* q, void* d);

/**
 * @brief Retrieve the front of the queue
 * 
 * @param q the queue to retrieve from
 * @return void*, the data on the front.
 */
void* queue_front(queue* q);

/**
 * @brief Pop an element from the queue.
 * 
 * @param q the queue to pop from
 * @return void*, the data popped
 */
void* queue_pop(queue* q);

#endif