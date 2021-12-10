#ifndef HETERO_H
#define HETERO_H

#include "hash.h"
#include<stdlib.h>
#include<assert.h>
//* KEY and VALUES are of of char string type.
#define KEY_LEN 16
#define VAL_LEN 16
typedef struct{
    uint8_t key[KEY_LEN];
    uint8_t value[VAL_LEN];
}entry;

typedef struct{
    uint64_t size;
    uint64_t count;
    entry* entries;
}index_sys;

int index_sys_init(index_sys* index, uint64_t size);

// Below are basic CRUD APIs, I mean, what could go wrong?
int create(index_sys* index, const uint8_t* key, const uint8_t* value, int storage_type);
uint8_t* query(index_sys* index, const uint8_t* key);
int update(index_sys* index, const uint8_t* key, const uint8_t* value);
int remove(index_sys* index, const uint8_t* key);

// don't know if we will implement these, but just put it here for now.
uint8_t** range_query(index_sys* index, uint8_t* key_begin, uint8_t* key_end);

// TODO: Do we need/allow explicit user control of the storage type for a certain key-value pair?
int move(index_sys* index, uint8_t* key, int storage_type);

#endif