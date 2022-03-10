#ifndef MURMUR3_H
#define MURMUR3_H
// Code adapted from https://github.com/PeterScott/murmur3/blob/master/murmur3.h
#include <stdint.h>

__BEGIN_DECLS
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

/**
 * @brief 128-bit Murmur3 hash 
 * 
 * @param key the key to compute on
 * @param len total length of the data
 * @param seed the seed for hash computation
 * @param out At least 128-bit space to hold the outcome. 
 *            Note that the function will treat this space as 2 uint64_t's. 
 */
void MurmurHash3_x64_128 ( const void * key, const int len,
                           const uint32_t seed, void* out);
__END_DECLS

#endif // MURMUR3_H
