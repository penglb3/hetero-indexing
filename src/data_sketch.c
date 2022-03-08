#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "murmur3.h"
#include "atomic.h"
#include "data_sketch.h"
#include "common.h"

sketch* sketch_construct(uint32_t width, uint32_t depth, uint32_t seed, int mode){
    sketch* sk = calloc(1, sizeof(sketch));
    if(!sk || (width & (width-1))) // ENFORCES that width is a power of 2
        return NULL;
    sk->width = width;
    sk->depth = depth;
    if(mode == BLOOM_FILTER && width >= sizeof(*sk->counts)*8 ){
        sk->counts = calloc(width / (sizeof(uint32_t)*8), sizeof(uint32_t));
    }
    else if(mode == COUNT_MIN){
        sk->counts = calloc(depth * width, sizeof(__typeof__(*sk->counts)));
        sk->shift = __builtin_ctz(sk->width);
        if(sk->shift * sk->depth > 128)
            return NULL;
    }
    else 
        return NULL;
    if(!sk->counts)
        return NULL;
    if(!seed){
        srand(time(NULL));
        sk->seed = rand();
    }
    return sk;
};

void sketch_destroy(sketch* sk){
    free(sk->counts);
    free(sk);
};

static inline uint32_t min(uint32_t a, uint32_t b){
    return a <= b ? a : b;
}

int countmin_inc_explicit(sketch* cm, const void* data, uint32_t len, void* ext_hash){
    uint32_t idx, minimal = UINT32_MAX, tot_shift = 0;
    uint64_t hash[2];
    if(IS_SPECIAL_KEY(data)) // For special key, especially OCCUPIED_FLAG, 
        return INT32_MAX; // We don't want caller to move them down, so return INT_MAX
    if(!ext_hash)
        MurmurHash3_x64_128(data, len, cm->seed, hash);
    else 
        hash[0] = ((uint64_t*)ext_hash)[0], hash[1] = ((uint64_t*)ext_hash)[1];
    for(int i=0; i<cm->depth; i++){
        idx = i * cm->width + (hash[0] & (cm->width - 1));
        cm->counts[idx]++;
        minimal = min(minimal, cm->counts[idx]);
        tot_shift += cm->shift;
        if(tot_shift >= 64){
            tot_shift -= 64;
            if(tot_shift == 0)
                hash[0] = hash[1];
            else{
                hash[0] |= (hash[1] << (cm->shift - tot_shift));
                hash[1] >>= (64 - tot_shift);
            }
        }
        hash[0] >>= cm->shift;
    }
    return minimal;
};

int countmin_count_explicit(sketch* cm, const void* data, uint32_t len, void* ext_hash){
    uint32_t minimal = UINT32_MAX, idx, tot_shift = 0;
    uint64_t hash[2];
    if(IS_SPECIAL_KEY(data)) // For special key, especially OCCUPIED_FLAG, 
        return INT32_MAX; // We don't want caller to move them down, so return INT_MAX
    if(!ext_hash)
        MurmurHash3_x64_128(data, len, cm->seed, hash);
    else 
        hash[0] = ((uint64_t*)ext_hash)[0], hash[1] = ((uint64_t*)ext_hash)[1];
    for(int i=0; i<cm->depth; i++){
        idx = i * cm->width + (hash[0] & (cm->width - 1));
        minimal = min(minimal, cm->counts[idx]);
        tot_shift += cm->shift;
        if(tot_shift >= 64){
            tot_shift -= 64;
            if(tot_shift == 0)
                hash[0] = hash[1];
            else{
                hash[0] |= (hash[1] << (cm->shift - tot_shift));
                hash[1] >>= (64 - tot_shift);
            }
        }
        hash[0] >>= cm->shift;
    }
    return minimal;
};

#define TEST_BIT(unit, i) (unit & (1<<(i)))

void bloom_add(sketch* b, void* data, uint32_t len){
    uint32_t idx, bit;
    uint64_t hash[2];
    MurmurHash3_x64_128(data, len, b->seed, hash);
    for(int i=0; i<b->depth; i++){
        // Calculate the hash index
        idx = MurmurHash3_x64_64(data, len, b->seed) & (b->width - 1); 
        bit = idx & (sizeof(*b->counts)*8 - 1);
        idx >>= (sizeof(*b->counts) + 1);
        SET_BIT(b->counts + idx, bit);
    }
}

int bloom_exists(sketch* b, void* data, uint32_t len){
    uint32_t idx, bit;
    for(int i=0; i<b->depth; i++){
        // Calculate the hash index
        idx = MurmurHash3_x64_64(data, len, b->seed) & (b->width - 1); 
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