#ifndef HETERO_H
#define HETERO_H

#include "hash.h"
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include <stdarg.h>
//* KEY and VALUES are of of char string type.
#define KEY_LEN 8
#define VAL_LEN 8
typedef struct{
    uint8_t key[KEY_LEN];
    uint8_t value[VAL_LEN];
} entry;

#define MEM_TYPE 1
#define IO_TYPE 2

#define BIN_CAPACITY 7
typedef struct entry_bin{
    uint8_t count;
    uint8_t status[BIN_CAPACITY]; //* 0=empty, 1=pmem, 2=io 
    struct entry_bin* next;
    entry data[BIN_CAPACITY];
} bin;

typedef struct index_sys_metadata{
    uint64_t seed;
    uint64_t size;
    uint64_t count;
    bin* entries;
} index_sys;

#define MAX_CHAIN_LEN 4
/*
Args:
> size:  # of bins of the indexing system, must be power of 2
> seed:  used for hash computation, 
         0 for random init, while non-zero value will be used for new index_sys.
Returns:
> A pointer pointing to the constructed index system.
*/
index_sys* index_construct(uint64_t size, uint64_t seed);
void index_destruct(index_sys* index);

int index_resize(index_sys* index, uint64_t new_size);
#define index_expand(ind) index_resize((ind), (ind)->size * 2)
#define index_shrink(ind) index_resize((ind), (ind)->size / 2)
#define SHRINK_THRESHOLD 0.8

// Basic CRUD APIs, I mean, what could go wrong?
int index_insert(index_sys* index, const uint8_t* key, const uint8_t* value, int storage_type);
uint8_t* index_query(index_sys* index, const uint8_t* key);
int index_update(index_sys* index, const uint8_t* key, const uint8_t* value);
int index_delete(index_sys* index, const uint8_t* key);

// don't know if we will implement these, but just put it here for now.
uint8_t** index_range_query(index_sys* index, uint8_t* key_begin, uint8_t* key_end);
int do_nothing(const char* format, ...);
int (*debug)(const char*, ...);
#endif // HETERO_H