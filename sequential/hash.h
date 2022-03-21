#ifndef HASH_H
#define HASH_H
#include "common.h"
#include "murmur3.h"
#include "data_sketch.h"
__BEGIN_DECLS
// --------------Hash System-----------------
int do_nothing(const char* format, ...);
int (*debug)(const char*, ...);

/**
* Construct a hash system
* @param size:    # of bins of the indexing system, must be power of 2
* @param seed:    used for hash computation, 
*               0 for random init, while non-zero value will be used for new hash_sys.
* @return a pointer pointing to the constructed index system.
*/
hash_sys* hash_construct(uint64_t size, uint32_t seed);

/**
* Destruct a hash system
* @param h:   the hash system to be destructed.
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
* @param h:       the INDEX system to operate on
* @param key:     the key you wish to find in / insert into the HASH system
* @param value:   the value to be inserted / updated
* @param mode:    one in [INSERT, UPDATE, STRICT_INSERT, STRICT_UPDATE], 
                (strict insert means you don't allow inplace update when the key exists, 
                 while the non-strict one does.)
* @return a int indicating the outcome. 0 = success, non-zero otherwise
*/
int hash_modify(index_sys* h, const uint8_t* key, const uint8_t* value, int* freq, int mode);
#define hash_insert(h, k, v) hash_modify((h), (k), (v), NULL, INSERT)
#define hash_update(h, k, v, fp) hash_modify((h), (k), (v), (fp), STRICT_UPDATE)
/**
* Insertion without check for duplicate.
* @param h:       the hash system to operate on
* @param key:     the key you wish to find in / insert into the system
* @param value:   the value to be inserted / updated
* @return a int indicating the outcome.
*/
int hash_insert_nocheck(hash_sys* hash_s, const uint8_t* key, const uint8_t* value);
/**
* Callbacks for query and delete when KEY IS FOUND in hash system
* @param h: The hash system to work on
* @param i: The bin's index in the table
* @param j: The key's offset in the bin
* @return the value w.r.t. the key for query, or constant 1 for delete
*/
const uint8_t* query_callback(hash_sys* h, entry* e);
const uint8_t* delete_callback(hash_sys* h, entry* e);
#define HASH_QUERY 1
#define HASH_DELETE 2
/**
* Unified search
* @param h:         the INDEX system to operate on
* @param key:       the key you wish to find in the HASH system
* @param freq:      a pointer to store the hash value of the key.
* @param query_result:    a pointer to store query's result value.
* @param mode:      one in [HASH_QUERY, HASH_DELETE].
* @return 0 if key not found, otherwise non-zero value indicating key found.
*/
int hash_search(
    index_sys* h, 
    const uint8_t* key, 
    uint64_t* freq, 
    uint64_t* query_result, 
    int mode
);
#define hash_query(i, key, freq, res) hash_search((i), (key), (freq), (res), HASH_QUERY)
#define hash_delete(i, key, h) hash_search((i), (key), (h), NULL, HASH_DELETE) 

/**
 * Hash expansion function pointer, should be either `hash_expand_copy` or `hash_expand_copy`
 */
int (*hash_expand)(hash_sys** h_ptr);

/**
 * Hash expansion by memcpy-ing the first half and picking out the other half.
 * @param h_ptr:  The address of the HASH system to be expanded.
 * @return 0 on success.
 */
int hash_expand_copy(hash_sys** h_ptr);

/**
 * Hash expansion by pure reinserting
 * @param h_ptr:  The address of the HASH system to be expanded.
 * @return 0 on success.
 */
int hash_expand_reinsert(hash_sys** h_ptr);

/**
 * @brief Compute a hash system's load factor
 * 
 * @param h: The hash system you wish to compute on
 * @return the load factor.
 */
double load_factor(hash_sys* h);
__END_DECLS
#endif // HASH_H