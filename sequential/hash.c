#include "hash.h"
#include "atomic.h"
#include "art.h"

int do_nothing(const char* format, ...){return 0;};
int (*debug)(const char*, ...);
int (*hash_expand)(hash_sys** h_ptr);


#define HASH_IDX(hash_s, h) (h[1] & ((hash_s)->size - 1))
#define HASH_COMP_IDX(hash_s, data) (MurmurHash3_x64_64((data), KEY_LEN, (hash_s)->seed) & ((hash_s)->size - 1))
#define HASH_LEVEL(hash_s, data) ((MurmurHash3_x64_64((data), KEY_LEN, (hash_s)->seed) & ((hash_s)->size)) != 0)

void hash_destruct(hash_sys* hash_s){
    free(hash_s->entries);
    free(hash_s);
}

static int hash_insert_nocheck_nosafe(hash_sys* hash_s, const uint8_t* key, const uint8_t* value){
    uint64_t idx = HASH_COMP_IDX(hash_s, key), key_i64 = *(uint64_t*)key, key_h;
    bin* target = hash_s->entries + idx;
    int j = 0;
    for(j = 0; j < BIN_CAPACITY; j++){
        if(*(uint64_t*)target->data[j].key == 0){
            *(uint64_t*)target->data[j].key = key_i64;
            hash_s->count++;
            *(uint64_t*)target->data[j].value = *(uint64_t*)value;
            return 0;
        }
    }
    return HASH_BIN_FULL;
}

hash_sys* hash_construct(uint64_t size, uint32_t seed){
    // allocate space for metadata structure
    hash_sys* hash_s = malloc(sizeof(hash_sys));
    if(!hash_s) 
        return NULL;
    if(seed==0){// generate seeds
        srand(time(NULL));
        hash_s->seed = rand();
    }
    else 
        hash_s->seed = seed;
    // initialize other members
    hash_s->size = size;
    hash_s->count = 0;
    if(size & (size - 1)) // Size must be power of 2
        return NULL;
    hash_s->entries = calloc(size, sizeof(bin));
    if(!hash_s->entries)
        return NULL;
    return hash_s;
}

int hash_expand_copy(hash_sys** hash_ptr){
    hash_sys* hash_s = *(hash_ptr), *new_h = hash_construct(hash_s->size * 2, hash_s->seed);
    bin* curr, *new_entries = new_h->entries;
    memcpy(new_entries, hash_s->entries, hash_s->size*sizeof(bin));
    // memcpy(new_entries + hash_s->size, hash_s->entries, hash_s->size*sizeof(bin));
    for(uint64_t i=0; i < hash_s->size; i++){
        curr = new_entries + i;
        for(int j=0, k=0; j<BIN_CAPACITY; j++){
            if(*(uint64_t*)curr->data[j].key && HASH_LEVEL(hash_s, curr->data[j].key)){
                *(uint64_t*)(curr+hash_s->size)->data[k].key = *(uint64_t*)curr->data[j].key;
                *(uint64_t*)(curr+hash_s->size)->data[k].value = *(uint64_t*)curr->data[j].value;
                *(uint64_t*)curr->data[j].key = 0;
                k++;
            }
        }
    }
    new_h->count = hash_s->count;
    *hash_ptr = new_h;
    hash_destruct(hash_s);
    return 0;
};

int hash_expand_reinsert(hash_sys** hash_ptr){
    hash_sys* hash_s = *(hash_ptr), *new_h = hash_construct(hash_s->size*2, hash_s->seed);
    bin* curr;
    for (uint64_t i = 0; i < hash_s->size; i++){
        curr = hash_s->entries + i;
        for (int j = 0; j < BIN_CAPACITY; j++){
            if (*(uint64_t *)curr->data[j].key)
                hash_insert_nocheck_nosafe(new_h, curr->data[j].key, curr->data[j].value);
        }
    }
    *hash_ptr = new_h;
    hash_destruct(hash_s);
    return 0;
};

int hash_modify(index_sys* ind, const uint8_t* key, const uint8_t* value, int* freq, int mode){
    hash_sys* hash_s = ind->hash;
    uint64_t h[2], comp = 0, key_i64 = *(uint64_t*)key, key_h, key_lru;
    MurmurHash3_x64_128(key, KEY_LEN, hash_s->seed, h);
    bin* target = hash_s->entries + HASH_IDX(hash_s, h);
    int j = 0, available = -1, i_min = -1, cnt = INT32_MAX;
    #ifdef SDR_SINK
    int min_cnt = (mode & INSERT) || (ind->sample_acc & (CM_SAMPLE_INTERVAL-1)) ? 0 : countmin_inc_explicit(ind->cm, (const void *)key, KEY_LEN, h);
    if(min_cnt && freq)
        *freq = min_cnt;
    #endif
    for(j = 0; j < BIN_CAPACITY; j++){ // Try INPLACE update
        key_h = *((uint64_t*)target->data[j].key);
        if(key_h == key_i64){ // FOUND the key!
            if(mode == STRICT_INSERT) // Inplace update not allowed in strict insert.
                return ELEMENT_ALREADY_EXISTS;
            // Inplace update and exit.
            *(uint64_t*)target->data[j].value = *(uint64_t*)value;
            return 0;
        }
        if(key_h == EMPTY_FLAG && available == -1)
            available = j;
        #ifdef SDR_SINK
        if(min_cnt > 0 && available == -1 && i_min == -1 && (rand() & (BIN_CAPACITY-1)) < SDR_SINK) {
            cnt = countmin_count(ind->cm, (const void*)target->data[j].key, KEY_LEN);
            cnt = countmin_amplify(cnt);
            if(min_cnt > cnt){
                min_cnt = cnt;
                i_min = j;
                key_lru = key_h;
            }
        }
        #endif
    }
    if(i_min != -1){ // No empty slot, but we do have an key_lru (that means we are to UPDATE)
        // Now put key_lru in ART.
        art_insert_no_replace(ind->tree, target->data[i_min].key, MEM_TYPE, (void*)target->data[i_min].value);
        // Since we have the key in the system, but not in hash table, then it must be in ART
        art_delete(ind->tree, key, KEY_LEN);
        *(uint64_t*)target->data[i_min].value = *(uint64_t*)value;
        *(uint64_t*)target->data[i_min].key = key_i64;
        return 0;
    }
    // Reaching this point means key is not in hash table.
    if(mode == STRICT_UPDATE) // In strict update, insertion is not allowed!
        return ELEMENT_NOT_FOUND; 
    if(available == -1 && i_min == -1)
        return HASH_BIN_FULL;
    // If there is an empty slot, then try insert. 
    if(available != -1)
        for(j = available; j < BIN_CAPACITY; j++){
            *(uint64_t*)target->data[j].value = *(uint64_t*)value;
            *(uint64_t*)target->data[j].key = key_i64;
            hash_s->count ++;
            return 0;
        }
    return HASH_BIN_FULL;
}

int hash_insert_nocheck(hash_sys* hash_s, const uint8_t* key, const uint8_t* value){
    uint64_t idx = HASH_COMP_IDX(hash_s, key), key_i64 = *(uint64_t*)key, empty;
    bin* target = hash_s->entries + idx;
    int j = 0;
    for(j = 0; j < BIN_CAPACITY; j++){
        empty = EMPTY_FLAG;
        if((uint64_t*)target->data[j].key == EMPTY_FLAG){
            *(uint64_t*)target->data[j].key = key_i64;
            *(uint64_t*)target->data[j].value = *(uint64_t*)value;
            hash_s->count ++;
            return 0;
        }
    }
    return HASH_BIN_FULL;
}

int hash_search(index_sys* ind, const uint8_t* key, uint64_t* h, uint64_t* query_result, int mode){
    hash_sys* hash_s = ind->hash;
    uint64_t key_i64 = *(uint64_t*)key;
    MurmurHash3_x64_128(key, KEY_LEN, hash_s->seed, h);
    bin* target = hash_s->entries + HASH_IDX(hash_s, h);
    for(int j=0; j<BIN_CAPACITY; j++){
        if(*((uint64_t*)target->data[j].key) == key_i64){
            if(mode == HASH_QUERY){
                *query_result = *((uint64_t*)target->data[j].value);
                return 1;
            }
            else {
                *(uint64_t*)target->data[j].key = EMPTY_FLAG;
                hash_s->count --;
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

double load_factor(hash_sys* h){
    return (double)h->count * BIN_CAPACITY / h->size;
}