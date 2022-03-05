#ifndef HETERO_H
#define HETERO_H
#include "common.h"
#include "art.h"
#include "hash.h"
#include "data_sketch.h"

// --------------Index System-----------------
#define CM_DEPTH 6
#define CM_WIDTH 1024
#define MEMB_WIDTH (1 << 13)
#define MEMB_DEPTH 7

index_sys* index_construct(uint64_t hash_size, uint64_t seed);
void index_destruct(index_sys* index);
int index_insert(index_sys* index, const uint8_t* key, const uint8_t* value, int storage_type);
uint8_t* index_query(index_sys* index, const uint8_t* key);
int index_update(index_sys* index, const uint8_t* key, const uint8_t* value);
int index_delete(index_sys* index, const uint8_t* key);
uint64_t index_size(index_sys* index);

#endif // HETERO_H