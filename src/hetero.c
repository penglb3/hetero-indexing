#include "hetero.h"

int do_nothing(const char* format, ...){return 0;};

#define HASH_IDX(hash_s, data) (MurmurHash3_x64_128((data), KEY_LEN, (hash_s)->seed) & ((hash_s)->size - 1))
#define HASH_LEVEL(hash_s, data) ((MurmurHash3_x64_128((data), KEY_LEN, (hash_s)->seed) & ((hash_s)->size)) != 0)
#define SET_BIT(unit, i) AO_OR_F((unit), (1<<(i)))
#define UNSET_BIT(unit, i) AO_AND_F(unit, (~(1<<(i))))
#define TEST_BIT(unit, i) (unit & (1<<(i)))

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
    hash_s->entries = aligned_alloc(64, size*sizeof(bin));
    hash_s->occupied = calloc(size, sizeof(binflag_t));
    if(!hash_s->entries || !hash_s->occupied) 
        return NULL;
    return hash_s;
}

int hash_expand_copy(hash_sys* hash_s){
    bin* curr, *new_entries = aligned_alloc(64, hash_s->size*sizeof(bin)*2);
    if(debug != do_nothing){
        log_stat(hash_s, count);
    }
    binflag_t temp, *new_occupied = malloc(hash_s->size*2*sizeof(binflag_t));
    memcpy(new_entries, hash_s->entries, hash_s->size*sizeof(bin));
    memcpy(new_entries + hash_s->size, hash_s->entries, hash_s->size*sizeof(bin));
    memcpy(new_occupied, hash_s->occupied, hash_s->size*sizeof(binflag_t));
    memcpy(new_occupied + hash_s->size, hash_s->occupied, hash_s->size*sizeof(binflag_t));
    
    for(uint64_t i=0; i<hash_s->size; i++){
        if(!hash_s->occupied[i]) continue;
        temp = 0;
        curr = hash_s->entries + i;
        for(int j=0; j<BIN_CAPACITY; j++){
            if(TEST_BIT(new_occupied[i], j) && HASH_LEVEL(hash_s, curr->data[j].key))
                temp |= (1<<j); 
        }
        new_occupied[i] &= ~temp;
        new_occupied[i + hash_s->size] &= temp;
    }
    free(hash_s->entries);
    hash_s->entries = new_entries;
    free(hash_s->occupied);
    hash_s->occupied = new_occupied;
    hash_s->size *= 2;
    if(debug != do_nothing){
        debug("[Debug] <!> hash_expand_copy with size %ld ends\n", hash_s->size);
        log_stat(hash_s, count+BIN_CAPACITY+1);
        show_stat(hash_s);
    }
    return 0;
};

int hash_expand_reinsert(hash_sys* hash_s){
    hash_sys* new_index = hash_construct(hash_s->size*2, hash_s->seed);
    if(debug != do_nothing){
        log_stat(hash_s, count);
    }
    bin* curr;
    for(uint64_t i=0; i<hash_s->size; i++){
        if(!hash_s->occupied[i]) continue;
        curr = hash_s->entries + i;
        for(int j=0; j<BIN_CAPACITY; j++){
            if(TEST_BIT(hash_s->occupied[i], j))
                hash_insert(new_index, curr->data[j].key, curr->data[j].value);
        }
    }
    free(hash_s->entries);
    hash_s->entries = new_index->entries;
    free(hash_s->occupied);
    hash_s->occupied = new_index->occupied;
    free(new_index);
    hash_s->size *= 2;
    if(debug != do_nothing){
        debug("[Debug] <!> hash_expand_copy with size %ld ends\n", hash_s->size);
        log_stat(hash_s, count+BIN_CAPACITY+1);
        show_stat(hash_s);
    }
    return 0;
};

int hash_modify(hash_sys* hash_s, const uint8_t* key, const uint8_t* value, int mode){
    // TODO: check for duplication?
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

uint8_t* hash_search(hash_sys* hash_s, const uint8_t* key, uint8_t* (*callback)(hash_sys*, uint64_t, int)){
    uint64_t key_int64 = *(uint64_t*)key, i = HASH_IDX(hash_s, key);
    bin* target = hash_s->entries + i;
    for(int j=0; j<BIN_CAPACITY; j++)
        if(TEST_BIT(hash_s->occupied[i], j) && *(uint64_t*)target->data[j].key == key_int64)
            return callback(hash_s, i, j);
    return NULL;
}

uint8_t* query_callback(hash_sys* hash_s, uint64_t i, int j){
    return hash_s->entries[i].data[j].value;
}

uint8_t* delete_callback(hash_sys* hash_s, uint64_t i, int j){
    UNSET_BIT(hash_s->occupied + i, j);
    hash_s->count--;
    return (void*)0x1;
}

// -------------- INDEX SYS --------------

index_sys* index_construct(uint64_t hash_size, uint64_t seed){
    index_sys* index = calloc(1, sizeof(index_sys));
    if(!index) 
        return NULL;
    index->hash = hash_construct(hash_size, seed);
    index->tree = calloc(1, sizeof(art_tree));
    index->cm = countmin_init(CM_WIDTH, CM_DEPTH);
    if(!index->hash || !index->tree || !index->cm) 
        return NULL;
    return art_tree_init(index->tree)? NULL : index;
}

void index_destruct(index_sys* index){
    hash_destruct(index->hash);
    art_tree_destroy(index->tree);
    countmin_destroy(index->cm);
    free(index->tree);
    free(index);
}

int index_insert(index_sys* index, const uint8_t* key, const uint8_t* value, int storage_type){
    int status;
    void* c = NULL;
    if(storage_type == MEM_TYPE){
    REINSERT:
        status = hash_modify(index->hash, key, value, INSERT);
        if(status == 0){
            return 0;
        }
        // if(status == HASH_BIN_FULL){
        //     hash_expand(index->hash);
        //     goto REINSERT;
        // }
    }
    return art_insert_no_replace(index->tree, key, KEY_LEN, (void*)value)!=NULL;
}

uint8_t* index_query(index_sys* index, const uint8_t* key){
    countmin_log(index->cm, (void*)key, KEY_LEN);
    uint8_t* result = hash_query(index->hash, key);
    if(result) 
        return result;
    return art_search(index->tree, key, KEY_LEN);
}

int index_update(index_sys* index, const uint8_t* key, const uint8_t* value){
    int status = hash_update(index->hash, key, value);
    if(status == ELEMENT_NOT_FOUND){
        return art_insert(index->tree, key, KEY_LEN, (void*)value) == NULL;
    }
    return 0;
}

int index_delete(index_sys* index, const uint8_t* key){
    int status = hash_delete(index->hash, key);
    if(status == ELEMENT_NOT_FOUND){
        return art_delete(index->tree, key, KEY_LEN) == NULL;
    }
    return 0;
}