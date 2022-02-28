#ifndef MURMUR3_H
#define MURMUR3_H
// Code adapted from https://github.com/PeterScott/murmur3/blob/master/murmur3.h
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
// Note that this function actually returns 64 bit value now.
uint64_t MurmurHash3_x64_128 ( const void * key, const int len,
                           const uint32_t seed);
#ifdef __cplusplus
}
#endif 

#endif // MURMUR3_H
