#include "hash.h"
#include "atomic.h"
#include "art.h"

int do_nothing(const char* format, ...){return 0;};

#define HASH_IDX(hash_s, h) (h[0] & ((hash_s)->size - 1))
#define HASH_COMP_IDX(hash_s, data) (MurmurHash3_x64_64((data), KEY_LEN, (hash_s)->seed) & ((hash_s)->size - 1))
#define HASH_LEVEL(hash_s, data) ((MurmurHash3_x64_64((data), KEY_LEN, (hash_s)->seed) & ((hash_s)->size)) != 0)

uint64_t count[BIN_CAPACITY*2+2] = {0};

static void log_stat(hash_sys* hash_s, uint64_t* count_){
    for(uint64_t i=0, c=0; i<hash_s->size; i++, c=0){
        for(int j=0; j<BIN_CAPACITY; j++)
            if(*(uint64_t*)hash_s->entries[i].data[j].key)
                c++;
        count_[c]++;
    }
}

static void show_stat(hash_sys* hash_s){
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
    hash_s->entries = calloc(size, sizeof(bin));
    if(!hash_s->entries)
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
    atomic_store(hash_ptr, new_h);
    hash_destruct(hash_s);
    if(debug != do_nothing){
        debug("[Debug] <!> hash_expand_copy with size %ld ends\n", new_h->size);
        log_stat(new_h, count+BIN_CAPACITY+1);
        show_stat(new_h);
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
                hash_insert_nocheck_nosafe(new_h, curr->data[j].key, curr->data[j].value);
        }
    }
    atomic_store(hash_ptr, new_h);
    hash_destruct(hash_s);
    if(debug != do_nothing){
        debug("[Debug] <!> hash_expand_copy with size %ld ends\n", new_h->size);
        log_stat(new_h, count+BIN_CAPACITY+1);
        show_stat(new_h);
    }
    return 0;
};

int hash_modify(index_sys* ind, const uint8_t* key, const uint8_t* value, int* freq, int mode){
    hash_sys* hash_s = ind->hash;
    uint64_t h[2], comp = 0, key_i64 = *(uint64_t*)key, key_h, key_lru;
    MurmurHash3_x64_128(key, KEY_LEN, hash_s->seed, h);
    bin* target = hash_s->entries + HASH_IDX(hash_s, h);
    int j = 0, available = -1, i_min = -1, cnt = INT32_MAX, 
        min_cnt = (mode & INSERT) ? 0 : countmin_inc_explicit(ind->cm, (const void *)key, KEY_LEN, h);
    if((mode & INSERT) == 0 && freq)
        *freq = min_cnt;
    for(j = 0; j < BIN_CAPACITY; j++){ // Try INPLACE update
        key_h = atomic_load((uint64_t*)target->data[j].key);
        if(key_h == key_i64){ // FOUND the key!
            if(mode == STRICT_INSERT) // Inplace update not allowed in strict insert.
                return ELEMENT_ALREADY_EXISTS;
            // Inplace update and exit.
            atomic_store((uint64_t*)target->data[j].value, *(uint64_t*)value);
            return 0;
        }
        if(key_h == EMPTY_FLAG && available == -1)
            available = j;
        if(min_cnt && available == -1) {
            cnt = countmin_query(ind->cm, (const void*)target->data[j].value, KEY_LEN);
            if(min_cnt > cnt){
                min_cnt = cnt;
                i_min = j;
                key_lru = key_h;
            }
        }
    }
    // Reaching this point means key is not in hash table.
    if(mode == STRICT_UPDATE) // In strict update, insertion is not allowed!
        return ELEMENT_NOT_FOUND; 
    if(available == -1 && i_min == -1)
        return HASH_BIN_FULL;
    // If there is an empty slot, then try insert. 
    if(available != -1)
        for(j = available; j < BIN_CAPACITY; j++){
            comp = EMPTY_FLAG;
            if(atomic_compare_exchange_strong((uint64_t*)target->data[j].key, &comp, key_i64)){
                atomic_fetch_add(& hash_s->count, 1);
                atomic_store((uint64_t*)target->data[j].value, *(uint64_t*)value);
                return 0;
            }
        }
    else if(i_min != -1){ // No empty slot, but we do have an key_lru (that means we are to UPDATE)
        if(atomic_compare_exchange_strong((uint64_t*)target->data[i_min].key, &key_lru, OCCUPIED_FLAG)){
            // No one's gonna use h[] now, feel free to use it!
            h[0] = atomic_exchange((uint64_t*)target->data[i_min].value, *(uint64_t*)value);
            h[1] = OCCUPIED_FLAG; 
            if(atomic_compare_exchange_strong((uint64_t*)target->data[i_min].key, h+1, key_i64)){
                // Since we have the key in the system, but not in hash table, then it must be in ART
                art_delete(ind->tree, key, KEY_LEN);
                // Now put key_lru in ART.
                art_insert_no_replace(ind->tree, (void*)&key_lru, MEM_TYPE, (void*)h);
                return 0;
            }
            return -1; // Normally this shouldn't happen.
        }
    }
    return HASH_BIN_FULL;
}

int hash_insert_nocheck(hash_sys* hash_s, const uint8_t* key, const uint8_t* value){
    uint64_t idx = HASH_COMP_IDX(hash_s, key), key_i64 = *(uint64_t*)key, key_h;
    bin* target = hash_s->entries + idx;
    int j = 0;
    for(j = 0; j < BIN_CAPACITY; j++){
        key_h = 0;
        if(atomic_compare_exchange_strong((uint64_t*)target->data[j].key, &key_h, key_i64)){
            atomic_store((uint64_t*)target->data[j].value, *(uint64_t*)value);
            atomic_fetch_add(& hash_s->count, 1);
            return 0;
        }
    }
    return HASH_BIN_FULL;
}

const uint8_t* hash_search(index_sys* ind, const uint8_t* key, int* freq, const uint8_t* (*callback)(hash_sys*, entry*)){
    hash_sys* hash_s = ind->hash;
    uint64_t key_i64 = *(uint64_t*)key, h[2];
    MurmurHash3_x64_128(key, KEY_LEN, hash_s->seed, h);
    if(callback == query_callback && freq)
        *freq = countmin_inc_explicit(ind->cm, key, KEY_LEN, h);
    bin* target = hash_s->entries + HASH_IDX(hash_s, h);
    for(int j=0; j<BIN_CAPACITY; j++){
        if(atomic_load((uint64_t*)target->data[j].key) == key_i64)
            return callback(hash_s, target->data + j);
    }
    return NULL;
}

const uint8_t* query_callback(hash_sys *h, entry* e){
    return e->value;
}

const uint8_t* delete_callback(hash_sys *h, entry* e){
    atomic_store((uint64_t*)e->key, 0);
    atomic_fetch_sub(& h->count, 1);
    return (void*)0x1;
}

double load_factor(hash_sys* h){
    return (double)h->count * BIN_CAPACITY / h->size;
}