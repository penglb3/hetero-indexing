#ifndef HASH_H
#define HASH_H
#include "common.h"
#include "murmur3.h"
#include "data_sketch.h"

// --------------Hash System-----------------
int do_nothing(const char* format, ...);
int (*debug)(const char*, ...);

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
#define STRICT_UPDATE 2
#define STRICT_INSERT 3

#define HASH_BIN_FULL 2
#define ELEMENT_NOT_FOUND 1
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
int hash_modify(index_sys* h, const uint8_t* key, const uint8_t* value, int mode);
#define hash_insert(h, k, v) hash_modify((h), (k), (v), INSERT)
#define hash_update(h, k, v) hash_modify((h), (k), (v), STRICT_UPDATE)

/**
* Callbacks for query and delete when KEY IS FOUND in hash system
* @arg h: The hash system to work on
* @arg i: The bin's index in the table
* @arg j: The key's offset in the bin
* @return the value w.r.t. the key for query, or constant 1 for delete
*/
uint8_t* query_callback(hash_sys* h, entry* e);
uint8_t* delete_callback(hash_sys* h, entry* e);
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
    uint8_t* (*callback)(hash_sys*, entry* e)
);
#define hash_query(hash_s, key) hash_search((hash_s), (key), query_callback)
#define hash_delete(hash_s, key) (hash_search((hash_s), (key), delete_callback) ? 0 : ELEMENT_NOT_FOUND)

int (*hash_expand)(hash_sys** h_ptr);
int hash_expand_copy(hash_sys** h_ptr);
int hash_expand_reinsert(hash_sys** h_ptr);

#endif // HASH_H