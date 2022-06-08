#include "quasi-dynamic.h"
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#ifdef TEST_STD
#include <unordered_map>
#endif

static inline int max(int a, int b){
    return a > b ? a : b;
}

static inline int min(int a, int b){
    return a < b ? a : b;
}

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
        #if CONFLICT_RESOLVE == CHAIN
        for(int j=0; j<qd->seg[i].width; j++)
            free(qd->seg[i].content[j].stash);
        #endif
        free(qd->seg[i].content);
    }
    free(qd);
    return 0;
}

// ==================== General Index Process =========================

#define insert_proc(q, sid, b, k, v, cap) \
    for(int i=0; i<cap; i++) {\
        if((b)[i].key == EMPTY_TAG){\
            (b)[i].key = (k);\
            (b)[i].value = (v);\
            (q)->size++;\
            (q)->seg[sid].size++;\
            return 0;\
        } \
    }

#define update_proc(q, sid, b, k, v, cap) \
    for(int i=0; i<cap; i++){\
        if((b)[i].key == (k)){\
            (b)[i].value = (v);\
            return 0;\
        }\
    }

#define query_proc(q, sid, b, k, vptr, cap) \
    for(int i=0; i<cap; i++){\
        if((b)[i].key == k){\
            *(vptr) = (b)[i].value;\
            return 0;\
        }\
    }

#define delete_proc(q, sid, b, k, v, cap) \
    for(int i=0; i<cap; i++){\
        if((b)[i].key == k){\
            (b)[i].key = EMPTY_TAG;\
            (q)->size --;\
            (q)->seg[sid].size --;\
            return 0;\
        }\
    }

#define expand_proc(q, sid, b, k_, v, cap)\
    for(k=0; k<cap; k++){\
        if((b)[k].key == EMPTY_TAG){\
            (b)[k].key = (k_);\
            (b)[k].value = (v);\
            break;\
        } \
    }\
    if (k<cap) continue;

// ==================== Conflict Resolutions =========================

#define try_linear_resolve(q, t, bid, sid, k, v, cb_proc)\
    for(int id=max(0,(bid)-RADIUS); id<min((q)->seg[sid].width,(bid)+RADIUS+1); id++){\
        if(id == (bid)) continue;\
        t = qd->seg[sid].content + id;\
        cb_proc(q, sid, (t)->data, k, v, BIN_CAPACITY);\
    }

#define try_chain_resolve_insert(q, t, bid, sid, k, v, cb_proc)\
    if(!t->stash) {\
        t->stash = (entry*) calloc(DEFAULT_STASH_SIZE, sizeof(entry));\
        t->stash_size = DEFAULT_STASH_SIZE;\
    }\
    cb_proc(q, sid, t->stash, k, v, t->stash_size);\
    if(t->stash_size < MAX_STASH_SIZE) {\
        entry* tmp = (entry*) calloc(t->stash_size*2, sizeof(entry));\
        memcpy(tmp, t->stash, t->stash_size * sizeof(entry));\
        t->stash_size *= 2;\
        free(t->stash);\
        t->stash = tmp;\
        cb_proc(q, sid, t->stash, k, v, t->stash_size);\
    }

#define try_chain_resolve(q, t, bid, sid, k, v, cb_proc)\
    if(t->stash){\
        cb_proc(q, sid, t->stash, k, v, t->stash_size);\
    }

#define try_cuckoo_resolve(q, t, bid, sid, k, v, cb_proc)\
    bid = h >> (__builtin_ctz((q)->length) + __builtin_ctz((q)->seg[sid].width));\
    bid &= (q)->seg[sid].width - 1;\
    t = (q)->seg[sid].content + bid;\
    cb_proc(q, sid, (t)->data, k, v, BIN_CAPACITY);

#if CONFLICT_RESOLVE == NONE
#define try_resolve(q, t, bid, sid, k, v, cb_proc) 
#define try_resolve_insert(q, t, bid, sid, k, v, cb_proc)
#elif CONFLICT_RESOLVE == LINEAR
#define try_resolve try_linear_resolve
#define try_resolve_insert try_linear_resolve
#elif CONFLICT_RESOLVE == CHAIN
#define try_resolve try_chain_resolve
#define try_resolve_insert try_chain_resolve_insert
#elif CONFLICT_RESOLVE == REHASH
#define try_resolve try_cuckoo_resolve
#define try_resolve_insert try_cuckoo_resolve
#endif

// ========================= Index Functions =========================

int qd_hash_set(qd_hash* qd, uintptr_t key, uintptr_t val, int insert){
    assert(key != EMPTY_TAG);
    uint64_t h = MurmurHash3_x64_64(&key, KEY_LEN, qd->seed);
    bin* target;
    int seg_id = h & (qd->length - 1), bin_id, expand_chances = CONFLICT_RESOLVE != CHAIN ? 1 : qd_load_factor_seg(qd->seg[seg_id]) >= 1;
REINSERT:
    bin_id = h >> __builtin_ctz(qd->length);
    bin_id &= qd->seg[seg_id].width - 1;
    target = qd->seg[seg_id].content + bin_id;
    if(insert){
        insert_proc(qd, seg_id, target->data, key, val, BIN_CAPACITY);
        try_resolve_insert(qd, target, bin_id, seg_id, key, val, insert_proc);
        if(expand_chances > 0){
            qd_hash_expand(qd, seg_id);
            expand_chances--;
            goto REINSERT;
        }
    }
    else{
        update_proc(qd, seg_id, target->data, key, val, BIN_CAPACITY);
        try_resolve(qd, target, bin_id, seg_id, key, val, update_proc);
    }
    // Insertion failed (update never fails if key in hash)
    bin_id = h >> __builtin_ctz(qd->length);
    bin_id &= qd->seg[seg_id].width - 1;
    return 1 + ((bin_id<RADIUS)<<1) + ((bin_id+RADIUS>=qd->seg[seg_id].width)<<2);
}

int qd_hash_get(qd_hash* qd, uintptr_t key, uintptr_t* result){
    assert(key != EMPTY_TAG);
    uint64_t h = MurmurHash3_x64_64(&key, KEY_LEN, qd->seed),
        seg_id = h & (qd->length - 1), bin_id = h >> __builtin_ctz(qd->length);
    bin_id &= qd->seg[seg_id].width - 1;
    bin* target = qd->seg[seg_id].content + bin_id;
    query_proc(qd, seg_id, target->data, key, result, BIN_CAPACITY);
    try_resolve(qd, target, bin_id, seg_id, key, result, query_proc);
    return 1;
};

int qd_hash_del(qd_hash* qd, uintptr_t key){
    assert(key != EMPTY_TAG);
    uint64_t h = MurmurHash3_x64_64(&key, KEY_LEN, qd->seed),
        seg_id = h & (qd->length - 1), bin_id = h >> __builtin_ctz(qd->length);
    bin_id &= qd->seg[seg_id].width - 1;
    bin* target = qd->seg[seg_id].content + bin_id;
    delete_proc(qd, seg_id, target->data, key, NULL, BIN_CAPACITY);
    try_resolve(qd, target, bin_id, seg_id, key, NULL, delete_proc);
    return 1;
};

int qd_hash_expand(qd_hash* qd, int seg_id){
    uint64_t bin_id, width = qd->seg[seg_id].width, h;
    qd->seg[seg_id].width *= 2;
    int slen = __builtin_ctz(qd->length), k;
    bin *new_content = (bin*) calloc(qd->seg[seg_id].width, sizeof(bin)),
        *old = qd->seg[seg_id].content, 
        *new_bin;
    for(int l=0; l<width; l++){
        for(int j=0; j<BIN_CAPACITY; j++){
            if(old->data[j].key == EMPTY_TAG) continue;
            h = MurmurHash3_x64_64(&(old->data[j].key), KEY_LEN, qd->seed);
            bin_id = h >> slen;
            bin_id &= qd->seg[seg_id].width - 1;
            new_bin = new_content + bin_id;
            #if !CONFLICT_RESOLVE || CONFLICT_RESOLVE == CHAIN
            new_bin->data[j].key = old->data[j].key;
            new_bin->data[j].value = old->data[j].value;
            #else 
            expand_proc(qd, seg_id, new_bin->data, old->data[j].key, old->data[j].value, BIN_CAPACITY);
            #endif
            // From here we need to apply conflict resolution
            #if CONFLICT_RESOLVE == LINEAR
            for(int id=max(0,bin_id-RADIUS); id<min(qd->seg[seg_id].width, bin_id+RADIUS+1); id++){
                if(id == bin_id) continue;
                new_bin = new_content + id; // THE VERY LINE STOPPING US FROM USING try_resolve
                for(k=0; k<BIN_CAPACITY; k++){
                    if(new_bin->data[k].key != EMPTY_TAG) continue;
                    new_bin->data[k].key = old->data[j].key;
                    new_bin->data[k].value = old->data[j].value;
                    break;
                }
                if(k<BIN_CAPACITY) break;
            }
            #elif CONFLICT_RESOLVE == REHASH
             // From here we need to apply conflict resolution
            bin_id = h >> (__builtin_ctz(qd->length) + __builtin_ctz(qd->seg[seg_id].width));
            bin_id &= qd->seg[seg_id].width - 1;
            new_bin = new_content + bin_id; // THE VERY LINE STOPPING US FROM USING try_resolve
            expand_proc(qd, seg_id, new_bin->data, old->data[j].key, old->data[j].value, BIN_CAPACITY);
            #endif
        }
        #if CONFLICT_RESOLVE == CHAIN
        if(old->stash){
            for(int j=0; j<old->stash_size; j++){
                if(old->stash[j].key == EMPTY_TAG) continue;
                h = MurmurHash3_x64_64(&(old->stash[j].key), KEY_LEN, qd->seed);
                bin_id = h >> slen;
                bin_id &= qd->seg[seg_id].width - 1;
                new_bin = new_content + bin_id;
                expand_proc(qd, seg_id, new_bin->data, old->stash[j].key, old->stash[j].value, BIN_CAPACITY);
                try_chain_resolve_insert(qd, new_bin, bin_id, seg_id, old->stash[j].key, old->stash[j].value, expand_proc);
            }
            free(old->stash);
        }
        #endif
        old++;
    }
    free(qd->seg[seg_id].content);
    qd->seg[seg_id].content = new_content;
    // printf("! S%u: %lu", seg_id, qd->seg[seg_id].width);
    return 0;
}

double qd_load_factor(qd_hash* qd){
    unsigned int tot_cap = 0;
    for (int i=0; i<qd->length; i++){
        tot_cap += qd->seg[i].width;
    }
    return (double) qd->size / (tot_cap * BIN_CAPACITY);
}

double qd_load_factor_seg(segment s){
    return (double) s.size / (s.width * BIN_CAPACITY);
}

#ifdef STANDALONE

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Quick test for validation and debugging.
int main(int argc, char* argv[]){
    uint64_t seed = 0, num_entries = 4000000, length = 16, width = 1<<15;
    int error = 0;
    double lf = -1;
    uint64_t key, val;
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

    printf("[Insert] ");
    clock_t start = clock(), finish;
    for(uint64_t key=1; key<=num_entries; key++){
        #ifndef TEST_STD
        error = qd_hash_insert(qd, key, key);
        #else
        s.emplace(key, key);
        #endif
        if(error){
            printf(ANSI_COLOR_RED "key-value pair <%lu,%lu> failed." ANSI_COLOR_RESET "\n", key, key);
            error = 1;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lf s. # of entries in indexing system:%lu. Load factor:%.3lf\n", 
        (double)finish / CLOCKS_PER_SEC, qd->size, qd_load_factor(qd));
    
    printf("[Query]  ");
    start = clock();
    for(uint64_t key=1; key<=num_entries; key++){
        val = 0;
        #ifndef TEST_STD
        error = qd_hash_get(qd, key, &val);
        #else
        val = s[key];
        #endif
        if(error || val != key){
            printf(ANSI_COLOR_RED "key <%lu,%lu> failed. error code=%d" ANSI_COLOR_RESET "\n", key, val, error);
            error = 2;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lf s. \n", (double)finish / CLOCKS_PER_SEC);

    printf("[Update] ");
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
            printf(ANSI_COLOR_RED "key <%lu,%lu> failed. error code=%d" ANSI_COLOR_RESET "\n", key, val, error);
            error = 3;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lf s. \n", (double)finish / CLOCKS_PER_SEC);

    printf("[Remove] ");
    start = clock();
    for(uint64_t key=1; key<=num_entries; key++){
        #ifndef TEST_STD
        error = qd_hash_del(qd, key);
        #endif
        if(error){
            printf(ANSI_COLOR_RED "key <%lu> failed." ANSI_COLOR_RESET "\n", key);
            error = 4;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lf s. \n", (double)finish / CLOCKS_PER_SEC);

EXIT:
    printf("End size = %lu\n", qd->size);
    qd_hash_destroy(qd);
    return 0;
}
#endif

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

  return h2^h1; 
}
