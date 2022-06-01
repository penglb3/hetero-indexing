#include "quasi-dynamic.h"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#ifdef TEST_STD
#include <unordered_map>
#endif
qd_hash* qd_hash_init(int length, int width, int seed){
    if(length & (length-1) || width & (width-1))
        return NULL;
    qd_hash* qd = (qd_hash*) malloc( sizeof(segment) * length + sizeof(qd_hash) );
    if(!qd) return NULL;
    qd->length = length;
    qd->size = 0;
    qd->seed = seed;
    if(seed == 0){
        srand(time(NULL));
        qd->seed = rand();
    }
    for(int i=0; i<length; i++){
        qd->seg[i].width = width;
        qd->seg[i].content = (bin*) calloc( width, sizeof(bin) );
        if(!qd->seg[i].content) return NULL;
    }
    return qd;
}

int qd_hash_destroy(qd_hash* qd){
    for(int i=0; i<qd->length; i++){
        free(qd->seg[i].content);
    }
    free(qd);
    return 0;
}

int qd_hash_set(qd_hash* qd, uintptr_t key, uintptr_t val, int insert){
    assert(key != EMPTY_TAG);
    uint64_t h = MurmurHash3_x64_64(&key, KEY_LEN, qd->seed);
    bin* target;
    int seg_id = h & (qd->length - 1), bin_id, after_expand = 0;
REINSERT:
    bin_id = h >> __builtin_ctz(qd->length);
    bin_id &= qd->seg[seg_id].width - 1;
    target = qd->seg[seg_id].content + bin_id;
    if(insert){
        for(int i=0; i<BIN_CAPACITY; i++){
            if(target->data[i].key == EMPTY_TAG){
                target->data[i].key = key;
                target->data[i].value = val;
                qd->size++;
                return 0;
            }
        }
        if(!after_expand){
            qd_hash_expand(qd, seg_id);
            after_expand = 1;
            goto REINSERT;
        }
    }
    else{
        for(int i=0; i<BIN_CAPACITY; i++){
            if(target->data[i].key == key){
                target->data[i].value = val;
                return 0;
            }
        }
    }
    return 1;
}

int qd_hash_get(qd_hash* qd, uintptr_t key, uintptr_t* result){
    assert(key != EMPTY_TAG);
    uint64_t h = MurmurHash3_x64_64(&key, KEY_LEN, qd->seed),
        seg_id = h & (qd->length - 1), bin_id = h >> __builtin_ctz(qd->length);
    bin_id &= qd->seg[seg_id].width - 1;
    bin* target = qd->seg[seg_id].content + bin_id;
    for(int i=0; i<BIN_CAPACITY; i++){
        if(target->data[i].key == key){
            *result = target->data[i].value;
            return 0;
        }
    }
    return 1;
};

int qd_hash_del(qd_hash* qd, uintptr_t key){
    assert(key != EMPTY_TAG);
    uint64_t h = MurmurHash3_x64_64(&key, KEY_LEN, qd->seed),
        seg_id = h & (qd->length - 1), bin_id = h >> __builtin_ctz(qd->length);
    bin_id &= qd->seg[seg_id].width - 1;
    bin* target = qd->seg[seg_id].content + bin_id;
    for(int i=0; i<BIN_CAPACITY; i++){
        if(target->data[i].key == key){
            target->data[i].key = EMPTY_TAG;
            qd->size --;
            return 0;
        }
    }
    return 1;
};

int qd_hash_expand(qd_hash* qd, int seg_id){
    uint64_t bin_id, width = qd->seg[seg_id].width * 2;
    int slen = __builtin_ctz(qd->length);
    bin *new_content = (bin*) calloc(width, sizeof(bin)),
        *old = qd->seg[seg_id].content, 
        *new_bin;
    for(int i=0; i<qd->seg[seg_id].width; i++){
        for(int j=0; j<BIN_CAPACITY; j++){
            if(old->data[j].key == EMPTY_TAG) continue;
            bin_id = MurmurHash3_x64_64(&(old->data[j].key), KEY_LEN, qd->seed) >> slen;
            bin_id &= width - 1;
            new_bin = new_content + bin_id;
            new_bin->data[j].key = old->data[j].key;
            new_bin->data[j].value = old->data[j].value;
        }
        old++;
    }
    free(qd->seg[seg_id].content);
    qd->seg[seg_id].content = new_content;
    qd->seg[seg_id].width *= 2;
    // printf("! S%u: %lu", seg_id, qd->seg[seg_id].width);
    return 0;
}

int main(int argc, char* argv[]){
    uint64_t seed = 0, num_entries = 4000000, length = 16, width = 1<<15;
    int error;
    while((error = getopt(argc, argv, "s:l:w:n:"))!=-1)
        switch(error){
            case 'l': length = 1<<atoi(optarg); break;
            case 'w': width = 1<<atoi(optarg); break;
            case 's': seed = atoi(optarg);break;
            case 'n': num_entries = atoll(optarg);break;
            default : abort();
        }
    #ifdef TEST_STD
    std::unordered_map<uint64_t, uint64_t> s;
    #endif
    qd_hash* qd = qd_hash_init(length, width, seed);
    printf("Initial size = %ux%lux%d, # of samples = %lu, seed: %u\n", qd->length, width, BIN_CAPACITY, num_entries, qd->seed);

    uint64_t key, val;
    error = 0;
    clock_t start = clock(), finish;
    for(uint64_t key=1; key<=num_entries; key++){
        #ifndef TEST_STD
        error = qd_hash_insert(qd, key, key);
        #else
        s.emplace(key, key);
        #endif
        if(error){
            printf("!! insertion of key-value pair <%lu,%lu> failed. !!\n", key, key);
            error = 1;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Insert: Passed in %.3lf s. # of entries in indexing system:%lu\n", 
        (double)finish / CLOCKS_PER_SEC, qd->size);
    
    start = clock();
    for(uint64_t key=1; key<=num_entries; key++){
        val = 0;
        #ifndef TEST_STD
        error = qd_hash_get(qd, key, &val);
        #else
        val = s[key];
        #endif
        if(error || val != key){
            printf("!! query key <%lu,%lu> failed. error code=%d !!\n", key, val, error);
            error = 2;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Query: Passed in %.3lf s. \n", 
        (double)finish / CLOCKS_PER_SEC);

    start = clock();
    for(uint64_t key=1; key<=num_entries; key++){
        #ifndef TEST_STD
        error = qd_hash_update(qd, key, key*2+1);
        error = qd_hash_get(qd, key, &val);
        #else
        s[key] = key*2+1;
        val = s[key];
        #endif
        if(error || val != key*2+1){
            printf("!! update key <%lu,%lu> failed. !!\n", key, val);
            error = 3;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Update: Passed in %.3lf s. \n", 
        (double)finish / CLOCKS_PER_SEC);

    start = clock();
    for(uint64_t key=1; key<=num_entries; key++){
        #ifndef TEST_STD
        error = qd_hash_del(qd, key);
        #endif
        if(error){
            printf("!! delete key <%lu> failed. !!\n", key);
            error = 4;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Remove: Passed in %.3lf s. \n", 
        (double)finish / CLOCKS_PER_SEC);


EXIT:
    printf("End size = %lu\n", qd->size);
    qd_hash_destroy(qd);
    return 0;
}


// ===================== Murmur3 Hash ===========================

#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

static FORCE_INLINE uint32_t rotl32 ( uint32_t x, int8_t r )
{
  return (x << r) | (x >> (32 - r));
}

static FORCE_INLINE uint64_t rotl64 ( uint64_t x, int8_t r )
{
  return (x << r) | (x >> (64 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)
#define ROTL64(x,y)	rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

#define getblock(p, i) (p[i])

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

static FORCE_INLINE uint32_t fmix32 ( uint32_t h )
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

//----------

static FORCE_INLINE uint64_t fmix64 ( uint64_t k )
{
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

uint64_t MurmurHash3_x64_64 ( const void * key, const int len,
                           const uint32_t seed)
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;
  int i;

  uint64_t h1 = seed;
  uint64_t h2 = seed;

  uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
  uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const uint64_t * blocks = (const uint64_t *)(data);

  for(i = 0; i < nblocks; i++)
  {
    uint64_t k1 = getblock(blocks,i*2+0);
    uint64_t k2 = getblock(blocks,i*2+1);

    k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch(len & 15)
  {
  case 15: k2 ^= (uint64_t)(tail[14]) << 48;
  case 14: k2 ^= (uint64_t)(tail[13]) << 40;
  case 13: k2 ^= (uint64_t)(tail[12]) << 32;
  case 12: k2 ^= (uint64_t)(tail[11]) << 24;
  case 11: k2 ^= (uint64_t)(tail[10]) << 16;
  case 10: k2 ^= (uint64_t)(tail[ 9]) << 8;
  case  9: k2 ^= (uint64_t)(tail[ 8]) << 0;
           k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

  case  8: k1 ^= (uint64_t)(tail[ 7]) << 56;
  case  7: k1 ^= (uint64_t)(tail[ 6]) << 48;
  case  6: k1 ^= (uint64_t)(tail[ 5]) << 40;
  case  5: k1 ^= (uint64_t)(tail[ 4]) << 32;
  case  4: k1 ^= (uint64_t)(tail[ 3]) << 24;
  case  3: k1 ^= (uint64_t)(tail[ 2]) << 16;
  case  2: k1 ^= (uint64_t)(tail[ 1]) << 8;
  case  1: k1 ^= (uint64_t)(tail[ 0]) << 0;
           k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len; h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  return h2; 
}
