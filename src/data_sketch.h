#ifndef DATA_SKETCH_H
#define DATA_SKETCH_H

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "murmur3.h"
#include "atomic.h"

typedef struct {
    uint32_t width, depth;
    uint32_t *seeds;
    uint32_t *counts;
} sketch;

#define COUNT_MIN 1
#define BLOOM_FILTER 2
sketch* sketch_init(uint32_t w, uint32_t d, int mode);
#define countmin_init(w, d) sketch_init((w), (d), COUNT_MIN)
#define bloom_init(w, d) sketch_init((w), (d), BLOOM_FILTER)

void sketch_destroy(sketch* cm);
#define countmin_destroy sketch_destroy
#define bloom_destroy sketch_destroy

int countmin_inc(sketch* cm, const void* data, uint32_t len);
int countmin_query(sketch* cm, const void* data, uint32_t len);

void bloom_add(sketch* b, void* data, uint32_t len);
int bloom_exists(sketch* b, void* data, uint32_t len);
void bloom_expand(sketch* b);

#endif // DATA_SKETCH_H