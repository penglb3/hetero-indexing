#ifndef HETERO_H
#define HETERO_H
#include "common.h"
#include "art.h"
#include "hash.h"
#include "data_sketch.h"
__BEGIN_DECLS
// --------------Index System-----------------
#define CM_DEPTH 6


/**
* Construct an index system
* @param hash_size:    # of bins of the indexing system, must be power of 2
* @param seed:    used for hash computation, 
*               0 for random init, while non-zero value will be used for new hash_sys.
* @return a pointer pointing to the constructed index system.
*/
index_sys* index_construct(uint64_t hash_size, uint64_t seed);

/**
* Destruct an index system
* @param index: the index system to be destructed.
*/
void index_destruct(index_sys* index);

/**
 * @brief Insert a KV pair into an index system
 * 
 * @param index the index system to insert into
 * @param key the key to be inserted
 * @param value the value associated with the key
 * @param storage_type one of [MEM_TYPE, SSD_TYPE, HDD_TYPE]
 * @return 0 on success, non-zero value otherwise.
 */
int index_insert(index_sys* index, const uint8_t* key, const uint8_t* value, int storage_type);

/**
 * @brief Search for a key's associated value.
 * 
 * @param index the index system to look into
 * @param key the key to look for
 * @param query_result a pointer to store query's result value.
 * @return 0 if key found, otherwise non-zero error code
 */
int index_query(index_sys* index, const uint8_t* key, uint64_t* query_result);

/**
 * @brief Update a key's associated value. 
 * The key should have been inserted before, otherwise we would turn to insert.
 * @param index the index system to look into
 * @param key the key to update on
 * @param value the new value you want to update for the key
 * @return 0 on success, non-zero error code otherwise.
 */
int index_update(index_sys* index, const uint8_t* key, const uint8_t* value);

/**
 * @brief Delete a key (actually the KV pair) from the system
 * 
 * @param index the index system to look into
 * @param key the key to be deleted
 * @return 0 on success, non-zero error code otherwise.
 */
int index_delete(index_sys* index, const uint8_t* key);

/**
 * @brief Calculate the total amount of KV pairs in the system
 * 
 * @param index the target index system 
 * @return uint64_t, the result.
 */
uint64_t index_size(index_sys* index);

/**
 * @brief Expand the index system, especially the hash subsystem.
 * 
 * @param index the index system to be expanded
 * @return 0 on success.
 */
int index_expand(index_sys* index);

/**
 * @brief Compact an index system. 
 * This is done by filling the hash system and lifting ART's INB from deeper layers to shallower ones.
 * @param index the index system to be compacted
 * @param max_load_factor the maximum load factor of the hash system if you don't want it full.
 * @return 0 on success.
 */
int index_compact(index_sys* index, double max_load_factor);
__END_DECLS
#endif // HETERO_H