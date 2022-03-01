#include "hash.h"
#include "atomic.h"

int do_nothing(const char* format, ...){return 0;};

#define HASH_IDX(hash_s, data) (MurmurHash3_x64_128((data), KEY_LEN, (hash_s)->seed) & ((hash_s)->size - 1))
#define HASH_LEVEL(hash_s, data) ((MurmurHash3_x64_128((data), KEY_LEN, (hash_s)->seed) & ((hash_s)->size)) != 0)
// #define SET_BIT(ptr, i) AO_OR_F((ptr), (1<<(i)))
// #define UNSET_BIT(ptr, i) AO_AND_F(ptr, (~(1<<(i))))
// #define TEST_BIT(unit, i) (unit & (1<<(i)))

uint64_t count[BIN_CAPACITY*2+2] = {0};

void log_stat(hash_sys* hash_s, uint64_t* count_){
    unsigned int table[256] = 
    { 
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 
    }; 
    for(uint64_t i=0; i<hash_s->size; i++)
        count_[table[hash_s->occupied[i]&0xff] + table[(hash_s->occupied[i]>>8)&0xff]]++;
}

void show_stat(hash_sys* hash_s){
    uint64_t sum = hash_s->size >> 1;
    debug("Load factor (before expand): %.3lf%\n", (double)(hash_s->count) / (hash_s->size * BIN_CAPACITY) * 200 );
    debug("BIN#\t: COUNT\n");
    for(int i=0; i<=BIN_CAPACITY; i++)
        debug("%d\t: %-10d -> %d\n", i, count[i], count[i+BIN_CAPACITY+1]), sum += count[i] - count[i+BIN_CAPACITY+1];
    if(sum)
        debug("[WARNING] before - after = %ld, which is not 0!\n", sum);
    debug("\n");
    memset(count, 0, sizeof(uint64_t)*(BIN_CAPACITY*2+2));
}

void hash_destruct(hash_sys* hash_s){
    free(hash_s->entries);
    free(hash_s->occupied);
    free(hash_s);
}

hash_sys* hash_construct(uint64_t size, uint64_t seed){
    // allocate space for metadata structure
    hash_sys* hash_s = malloc(sizeof(hash_sys));
    if(!hash_s) 
        return NULL;
    if(seed==0){// generate seeds
        srand(time(NULL));
        hash_s->seed = ((uint64_t)rand()<<32) | rand();
    }
    else 
        hash_s->seed = seed;
    // initialize other members
    hash_s->size = size;
    hash_s->count = 0;
    hash_s->entries = calloc(size, sizeof(bin));
    hash_s->occupied = calloc(size, sizeof(binflag_t));
    if(!hash_s->entries || !hash_s->occupied) 
        return NULL;
    return hash_s;
}

int hash_expand_copy(hash_sys** hash_ptr){
    hash_sys* hash_s = atomic_load(hash_ptr), *new_h = hash_construct(hash_s->size * 2, hash_s->seed);
    bin* curr, *new_entries = new_h->entries;
    if(debug != do_nothing){
        log_stat(hash_s, count);
    }
    memcpy(new_entries, hash_s->entries, hash_s->size*sizeof(bin));
    memcpy(new_entries + hash_s->size, hash_s->entries, hash_s->size*sizeof(bin));
    
    for(uint64_t i=0; i < hash_s->size; i++){
        curr = new_entries + i;
        for(int j=0; j<BIN_CAPACITY; j++){
            if(*(uint64_t*)curr->data[j].key != 0){
                if(HASH_LEVEL(hash_s, curr->data[j].key))
                    *(uint64_t*)curr->data[j].key = 0;
                else
                    *(uint64_t*)(curr+hash_s->size)->data[j].key = 0;
            }
        }
    }
    new_h->count = hash_s->count;
    atomic_store(hash_ptr, new_h);
    hash_destruct(hash_s);
    if(debug != do_nothing){
        debug("[Debug] <!> hash_expand_copy with size %ld ends\n", hash_s->size);
        log_stat(hash_s, count+BIN_CAPACITY+1);
        show_stat(hash_s);
    }
    return 0;
};

int hash_expand_reinsert(hash_sys** hash_ptr){
    hash_sys* hash_s = atomic_load(hash_ptr), *new_h = hash_construct(hash_s->size*2, hash_s->seed);
    if(debug != do_nothing){
        log_stat(hash_s, count);
    }
    bin* curr;
    for (uint64_t i = 0; i < hash_s->size; i++){
        curr = hash_s->entries + i;
        for (int j = 0; j < BIN_CAPACITY; j++){
            if (*(uint64_t *)curr->data[j].key)
                hash_insert(new_h, curr->data[j].key, curr->data[j].value);
        }
    }
    atomic_store(hash_ptr, new_h);
    hash_destruct(hash_s);
    if(debug != do_nothing){
        debug("[Debug] <!> hash_expand_copy with size %ld ends\n", hash_s->size);
        log_stat(hash_s, count+BIN_CAPACITY+1);
        show_stat(hash_s);
    }
    return 0;
};

int hash_modify_old(hash_sys* hash_s, const uint8_t* key, const uint8_t* value, int mode){
    uint64_t idx = HASH_IDX(hash_s, key);
    bin* target = hash_s->entries + idx;
    int j = 0;
    //check for existence
    for(j=0; j<BIN_CAPACITY; j++) {
        if(TEST_BIT(hash_s->occupied[idx], j) && *(uint64_t*)target->data[j].key == *(uint64_t*)key){
            break;
        }
    }
    if(mode == STRICT_UPDATE && j == BIN_CAPACITY)
        return ELEMENT_NOT_FOUND; // In strict update, insertion is not allowed!
    if(mode == STRICT_INSERT && j < BIN_CAPACITY)
        return ELEMENT_ALREADY_EXISTS; // In strict insertion, inplace update is not allowed!
    // ------------------- Update data -----------------------------------
    if(j == BIN_CAPACITY){ // When key is not found
        if(hash_s->occupied[idx]==FULL_FLAG) // and the bin is FULL,
            return HASH_BIN_FULL; // Please expand or put it somewhere else.
        // Otherwise just find a place to insert:
        for(j=0; j<BIN_CAPACITY && TEST_BIT(hash_s->occupied[idx], j); j++);
    }
    // Since the space is empty, we can directly write!
    *(uint64_t*)target->data[j].key = *(uint64_t*)key;
    *(uint64_t*)target->data[j].value = *(uint64_t*)value;
    // ------------------- Update metadata -------------------------------
    // TODO: the following actions need to be [CRITICAL]
    if(!TEST_BIT(hash_s->occupied[idx], j)){
        SET_BIT(hash_s->occupied + idx, j);
        hash_s->count++;
    }
    // TODO: [CRITICAL] ends
    return 0;
};

uint8_t* hash_search_old(hash_sys* hash_s, const uint8_t* key, uint8_t* (*callback)(hash_sys*, uint64_t, int)){
    uint64_t key_int64 = *(uint64_t*)key, i = HASH_IDX(hash_s, key);
    bin* target = hash_s->entries + i;
    for(int j=0; j<BIN_CAPACITY; j++)
        if(TEST_BIT(hash_s->occupied[i], j) && *(uint64_t*)target->data[j].key == key_int64)
            return callback(hash_s, i, j);
    return NULL;
}

uint8_t* query_callback_old(hash_sys* hash_s, uint64_t i, int j){
    return hash_s->entries[i].data[j].value;
}

uint8_t* delete_callback_old(hash_sys* hash_s, uint64_t i, int j){
    UNSET_BIT(hash_s->occupied + i, j);
    hash_s->count--;
    return (void*)0x1;
}

int hash_modify(hash_sys* hash_s, const uint8_t* key, const uint8_t* value, int mode){
    uint64_t idx = HASH_IDX(hash_s, key), comp = 0, key_i64 = *(uint64_t*)key, key_h;
    bin* target = hash_s->entries + idx;
    int j = 0, available = -1;
    for(j = 0; j < BIN_CAPACITY; j++){ // Try update
        key_h = atomic_load((uint64_t*)target->data[j].key);
        if(key_h == key_i64){
            if(mode == STRICT_INSERT)
                return ELEMENT_ALREADY_EXISTS;
            atomic_store((uint64_t*)target->data[j].value, *(uint64_t*)value);
            return 0;
        }
        if(key_h == 0 && available == -1)
            available = j;
    }
    if(mode == STRICT_UPDATE)
        return ELEMENT_NOT_FOUND; // In strict update, insertion is not allowed!
    if(available == -1)
        return HASH_BIN_FULL;
    for(j = available; j < BIN_CAPACITY; j++){
        comp = 0;
        if(atomic_compare_exchange_strong((uint64_t*)target->data[j].key, &comp, key_i64)){
            atomic_fetch_add(& hash_s->count, 1);
            atomic_store((uint64_t*)target->data[j].value, *(uint64_t*)value);
            return 0;
        }
    }
    return HASH_BIN_FULL;
}

uint8_t* hash_search(hash_sys* hash_s, const uint8_t* key, uint8_t* (*callback)(hash_sys*, entry*)){
    uint64_t key_i64 = *(uint64_t*)key, i = HASH_IDX(hash_s, key);
    bin* target = hash_s->entries + i;
    for(int j=0; j<BIN_CAPACITY; j++){
        if(atomic_load((uint64_t*)target->data[j].key) == key_i64)
            return callback(hash_s, target->data + j);
    }
    return NULL;
}

uint8_t* query_callback(hash_sys *h, entry* e){
    return e->value;
}

uint8_t* delete_callback(hash_sys *h, entry* e){
    atomic_store((uint64_t*)e->key, 0);
    atomic_fetch_sub(& h->count, 1);
    return (void*)0x1;
}