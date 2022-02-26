#ifndef HETERO_H
#define HETERO_H

#include<stdlib.h>
#include<string.h>
#include<time.h>
#include <stdarg.h>
#include "hash.h"
#include "atomic.h"
#include "art.h"

int do_nothing(const char* format, ...);
int (*debug)(const char*, ...);

#define MEM_TYPE 1
#define SSD_TYPE 2
#define HDD_TYPE 3
//* KEY and VALUES are of of char string type.
#define KEY_LEN 8
#define VAL_LEN 8
// --------------Hash System-----------------
typedef struct{
    uint8_t key[KEY_LEN];
    uint8_t value[VAL_LEN];
} entry;

typedef uint8_t binflag_t;
#define BIN_CAPACITY 8
#define FULL_FLAG (1<<BIN_CAPACITY) - 1

typedef struct entry_bin{
    entry data[BIN_CAPACITY];
} bin;

typedef struct hash_metadata{
    uint64_t seed;
    uint64_t size;
    uint64_t count;
    art_tree* tree;
    binflag_t* occupied;
    bin* entries;
} hash_sys;

/**
* Construct a hash system
* @arg size:    # of bins of the indexing system, must be power of 2
* @arg seed:    used for hash computation, 
*               0 for random init, while non-zero value will be used for new hash_sys.
* @return a pointer pointing to the constructed index system.
*/
hash_sys* hash_construct(uint64_t size, uint64_t seed);

/**
* Destruct a hash system
* @arg h:   the hash system to be destructed.
*/
void hash_destruct(hash_sys* h);


#define UPDATE 0
#define INSERT 1
#define STRICT 2
#define STRICT_UPDATE 2
#define STRICT_INSERT 3

#define HASH_BIN_FULL 1
#define ELEMENT_NOT_FOUND 2
#define ELEMENT_ALREADY_EXISTS 3
/**
* Unified modification 
* @arg h:       the hash system to operate on
* @arg key:     the key you wish to find in / insert into the system
* @arg value:   the value to be inserted / updated
* @arg mode:    one in [INSERT, UPDATE, STRICT_INSERT, STRICT_UPDATE], 
                (strict insert means you don't allow inplace update when the key exists, 
                 while the non-strict one does.)
* @return a int indicating the outcome.
*/
int hash_modify(hash_sys* h, const uint8_t* key, const uint8_t* value, int mode);
#define hash_insert(h, k, v) hash_modify((h), (k), (v), INSERT)
#define hash_update(h, k, v) hash_modify((h), (k), (v), STRICT_UPDATE)

/**
* Callbacks for query and delete when KEY IS FOUND in hash system
* @arg h: The hash system to work on
* @arg i: The bin's index in the table
* @arg j: The key's offset in the bin
* @return the value w.r.t. the key for query, or constant 1 for delete
*/
uint8_t* query_callback(hash_sys* h, uint64_t i, int j);
uint8_t* delete_callback(hash_sys* h, uint64_t i, int j);
/**
* Unified search
* @arg h:           the hash system to operate on
* @arg key:         the key you wish to find in the system
* @arg callback:    whatever you wish to do when you found the key at row @i, col @j. 
                    It returns a uint8*.
* @return NULL if @key is not found, otherwise returns a uint8* returned by hash_callback. 
*/
uint8_t* hash_search(
    hash_sys* h, 
    const uint8_t* key, 
    uint8_t* (*hash_callback)(hash_sys* h, uint64_t i, int j)
);
#define hash_query(hash_s, key) hash_search((hash_s), (key), query_callback)
#define hash_delete(hash_s, key) (hash_search((hash_s), (key), delete_callback) ? 0 : ELEMENT_NOT_FOUND)

int (*hash_expand)(hash_sys* h);
int hash_expand_copy(hash_sys* h);
int hash_expand_reinsert(hash_sys* h);


// --------------Index System-----------------
typedef struct index{
    hash_sys* hash;
    art_tree* tree;
} index_sys;

index_sys* index_construct(uint64_t hash_size, uint64_t seed);
void index_destruct(index_sys* index);
int index_insert(index_sys* index, const uint8_t* key, const uint8_t* value, int storage_type);
uint8_t* index_query(index_sys* index, const uint8_t* key);
int index_update(index_sys* index, const uint8_t* key, const uint8_t* value);
int index_delete(index_sys* index, const uint8_t* key);

#endif // HETERO_H