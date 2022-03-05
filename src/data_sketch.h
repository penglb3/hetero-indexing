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
sketch* sketch_init(uint32_t width, uint32_t depth, uint32_t seed, int mode);
#define countmin_init(w, d, s) sketch_init((w), (d), (s), COUNT_MIN)
#define bloom_init(w, d, s) sketch_init((w), (d), (s), BLOOM_FILTER)

void sketch_destroy(sketch* cm);
#define countmin_destroy sketch_destroy
#define bloom_destroy sketch_destroy

int countmin_inc_explicit(sketch* cm, const void* data, uint32_t len, void* ext_hash);
int countmin_query_explicit(sketch* cm, const void* data, uint32_t len, void* ext_hash);
#define countmin_inc(cm, data, len) countmin_inc_explicit((cm), (data), (len), NULL)
#define countmin_query(cm, data, len) countmin_query_explicit((cm), (data), (len), NULL)

#define countmin_clear(cm) memset((cm)->counts, 0, (cm)->depth * (cm)->width);

void bloom_add(sketch* b, void* data, uint32_t len);
int bloom_exists(sketch* b, void* data, uint32_t len);
void bloom_expand(sketch* b);

#endif // DATA_SKETCH_H