#ifndef QUASI_H
#define QUASI_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdalign.h>

__BEGIN_DECLS

#define KEY_LEN 8
#define VAL_LEN 8
#define BIN_CAPACITY 8

#define EMPTY_TAG 0

typedef struct entry{
    alignas(16) uintptr_t key, value;
} entry;

typedef struct entry_bin{
    entry data[BIN_CAPACITY];
} bin;

typedef struct s{
    uint64_t width;
    bin* content; 
} segment;

typedef struct {
    int length, seed;
    uint64_t size;
    segment seg[];
} qd_hash;

qd_hash* qd_hash_init(int length, int width, int seed);
int qd_hash_destroy(qd_hash* qd);

int qd_hash_set(qd_hash* qd, uintptr_t key, uintptr_t val, int insert);
#define qd_hash_insert(qd, k, v) qd_hash_set((qd), (k), (v), 1)
#define qd_hash_update(qd, k, v) qd_hash_set((qd), (k), (v), 0)
int qd_hash_get(qd_hash* qd, uintptr_t key, uintptr_t* result);
int qd_hash_del(qd_hash* qd, uintptr_t key);
int qd_hash_expand(qd_hash* qd, int seg_id);

/**
 * @brief 64-bit Murmur3 hash 
 * 
 * @param key the key to compute on
 * @param len total length of the data
 * @param seed the seed for hash computation
 * @return the 64-bit hash value
 */
uint64_t MurmurHash3_x64_64 ( const void * key, const int len,
                           const uint32_t seed);

__END_DECLS

#endif