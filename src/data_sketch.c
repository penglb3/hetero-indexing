#include "data_sketch.h"

sketch* sketch_init(uint32_t width, uint32_t depth, int mode){
    sketch* sk = calloc(1, sizeof(sketch));
    if(!sk || (width & (width-1))) // ENFORCES that width is a power of 2
        return NULL;
    sk->width = width;
    sk->depth = depth;
    if(mode == BLOOM_FILTER && width >= sizeof(*sk->counts)*8 )
        sk->counts = calloc(width / (sizeof(uint32_t)*8), sizeof(uint32_t));
    else if(mode == COUNT_MIN)
        sk->counts = calloc(depth * width, sizeof(uint32_t));
    else 
        return NULL;
    sk->seeds = malloc(depth * sizeof(uint32_t));
    if(!sk->counts || !sk->seeds)
        return NULL;
    srand(time(NULL));
    for(int i=0; i<depth; i++){
        sk->seeds[i] = rand();
    }
    return sk;
};

void sketch_destroy(sketch* sk){
    free(sk->counts);
    free(sk->seeds);
    free(sk);
};

static inline uint32_t min(uint32_t a, uint32_t b){
    return a <= b ? a : b;
}

int countmin_inc(sketch* cm, const void* data, uint32_t len){
    uint32_t idx, minimal = UINT32_MAX;
    for(int i=0; i<cm->depth; i++){
        idx = i * cm->width + (MurmurHash3_x64_128(data, len, cm->seeds[i]) & (cm->width - 1));
        minimal = min(minimal, ++(cm->counts[idx]));
    }
    return minimal;
};


int countmin_query(sketch* cm, const void* data, uint32_t len){
    uint32_t minimal = UINT32_MAX, idx;
    for(int i=0; i<cm->depth; i++){
        idx = i * cm->width + (MurmurHash3_x64_128(data, len, cm->seeds[i]) & (cm->width - 1));
        minimal = min(minimal, cm->counts[idx]);
    }
    return minimal;
};

#define TEST_BIT(unit, i) (unit & (1<<(i)))

void bloom_add(sketch* b, void* data, uint32_t len){
    uint32_t idx, bit;
    for(int i=0; i<b->depth; i++){
        // Calculate the hash index
        idx = MurmurHash3_x64_128(data, len, b->seeds[i]) & (b->width - 1); 
        bit = idx & (sizeof(*b->counts)*8 - 1);
        idx >>= (sizeof(*b->counts) + 1);
        SET_BIT(b->counts + idx, bit);
    }
}

int bloom_exists(sketch* b, void* data, uint32_t len){
    uint32_t idx, bit;
    for(int i=0; i<b->depth; i++){
        // Calculate the hash index
        idx = MurmurHash3_x64_128(data, len, b->seeds[i]) & (b->width - 1); 
        bit = idx & (sizeof(*b->counts)*8 - 1);
        idx >>= (sizeof(*b->counts) + 1);
        if(TEST_BIT(b->counts[idx], bit)==0)
            return 0;
    }
    return 1;
}

void bloom_expand(sketch* b){
    free(b->counts);
    b->width *= 2;
    // We will also clear the filter when we expand.
    b->counts = calloc(b->width / (sizeof(uint32_t)*8), sizeof(uint32_t));
}