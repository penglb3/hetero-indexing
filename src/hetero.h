#ifndef HETERO_H
#define HETERO_H

#include "hash.h"
typedef struct{
    // TODO
}index_sys;

//* KEY and VALUES are of of char string type.

// Below are basic CRUD APIs, I mean, what could go wrong?
int create(index_sys* index, uint8_t* key, uint8_t* value, int storage_type);
uint8_t* query(index_sys* index, uint8_t* key);
int update(index_sys* index, uint8_t* key, uint8_t* value);
int remove(index_sys* index, uint8_t* key);

// don't know if we will implement these, but just put it here for now.
uint8_t** range_query(index_sys* index, uint8_t* key_begin, uint8_t* key_end);

// TODO: Do we need/allow explicit user control of the storage type for a certain key-value pair?
int move(index_sys* index, uint8_t* key, int storage_type);

#endif