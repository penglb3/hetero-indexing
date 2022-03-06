#ifndef DATA_SKETCH_H
#define DATA_SKETCH_H
#include <stdint.h>
typedef struct {
    uint32_t width, depth;
    uint32_t seed;
    uint32_t shift;
    uint32_t *counts;
} sketch;

#define COUNT_MIN 1
#define BLOOM_FILTER 2

/**
 * @brief Construct a data sketch
 * 
 * @param width the width of the array. 
 * @param depth number of hash functions for the sketch. For Count-Min it also means the "height" of the array.
 * @param seed used for hash computation, 
*               0 for random init, while non-zero value will be directly used.
 * @param mode one in [COUNT_MIN, BLOOM_FILTER].
 * @return sketch*, the result of the sketch system.
 */
sketch* sketch_construct(uint32_t width, uint32_t depth, uint32_t seed, int mode);
#define countmin_construct(w, d, s) sketch_construct((w), (d), (s), COUNT_MIN)
#define bloom_construct(w, d, s) sketch_construct((w), (d), (s), BLOOM_FILTER)

/**
 * @brief Destroy a data sketch
 * 
 * @param cm the sketch to be destroyed.
 */
void sketch_destroy(sketch* cm);
#define countmin_destroy sketch_destroy
#define bloom_destroy sketch_destroy

/**
 * @brief Count-Min: increment the count for a given piece of data
 * 
 * @param cm the count-min sketch
 * @param data the data to log
 * @param len length of the data
 * @param ext_hash pointer to 128-bit hash value for the data. If NULL, will compute using cm->seed.
 * @return the count of the data BEFORE incrementing
 */
int countmin_inc_explicit(sketch* cm, const void* data, uint32_t len, void* ext_hash);

/**
 * @brief Count-Min: Count for a given piece of data
 * 
 * @param cm the count-min sketch
 * @param data the data to query
 * @param len length of the data
 * @param ext_hash pointer to 128-bit hash value for the data. If NULL, will compute using cm->seed.
 * @return the count of the data
 */
int countmin_query_explicit(sketch* cm, const void* data, uint32_t len, void* ext_hash);
#define countmin_inc(cm, data, len) countmin_inc_explicit((cm), (data), (len), NULL)
#define countmin_query(cm, data, len) countmin_query_explicit((cm), (data), (len), NULL)

#define countmin_clear(cm) memset((cm)->counts, 0, (cm)->depth * (cm)->width);

/**
 * @brief Bloom filter: add an element
 * 
 * @param b the bloom filter
 * @param data the element to be added
 * @param len the length of the data
 */
void bloom_add(sketch* b, void* data, uint32_t len);

/**
 * @brief Bloom filter: check if an element exists in the filter
 * 
 * 
 * @param b the bloom filter
 * @param data the element to be added
 * @param len the length of the data
 * @return 0 for definitely not exists, non-zero for probably exists
 */
int bloom_exists(sketch* b, void* data, uint32_t len);

/**
 * @brief Bloom filter: Expand the filter width.
 * 
 * @param b the bloom filter to be expanded. Note that it will be cleared.
 */
void bloom_expand(sketch* b);

#endif // DATA_SKETCH_H