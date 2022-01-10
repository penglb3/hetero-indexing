#include "hetero.h"
#include "atomic.h"
int do_nothing(const char* format, ...){return 0;};

#define HASH_IDX(index, data) (MurmurHash3_x64_128((data), KEY_LEN, (index)->seed) & ((index)->size - 1))
#define HASH_LEVEL(index, data) ((MurmurHash3_x64_128((data), KEY_LEN, (index)->seed) & ((index)->size)) != 0)
#define SET_BIT(unit, i) unit |= 1<<(i)
#define UNSET_BIT(unit, i) unit &= ~(1<<(i))
#define TEST_BIT(unit, i) (unit & (1<<(i)))

void index_destruct(index_sys* index){
    free(index->entries);
    free(index->occupied);
    free(index);
}
void stat(index_sys* index){
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
    int count[BIN_CAPACITY+1]={0}, sum=0;
    for(uint64_t i=0; i<index->size; i++)
        count[table[index->occupied[i]&0xff] + table[(index->occupied[i]>>8)&0xff]]++;
    debug("Utilization: %.3lf\n", (double)(index->count) / index->size);
    debug("BIN#\t: COUNT\n");
    for(int i=0; i<=BIN_CAPACITY; i++)
        debug("%d\t: %d\n", i, count[i]), sum += count[i];
    debug("ALL\t: %d\n\n", sum);
}
index_sys* index_construct(uint64_t size, uint64_t seed){
    // allocate space for metadata structure
    index_sys* index = malloc(sizeof(index_sys));
    if(!index) 
        return NULL;
    if(seed==0){// generate seeds
        srand(time(NULL));
        index->seed = ((uint64_t)rand()<<32) | rand();
    }
    else 
        index->seed = seed;
    // initialize other members
    index->entries = aligned_alloc(64, size*sizeof(bin));

    // TODO: 1. really need? 2. Any quicker way?
    index->size = size;
    index->count = 0;
    index->occupied = malloc(size*sizeof(BIN_FLAG_TYPE));
    if(!index->entries || !index->occupied) 
        return NULL;
    memset(index->occupied, 0, size*sizeof(BIN_FLAG_TYPE));
    return index;
}

int index_expand(index_sys* index){
    bin* curr, *new_entries = aligned_alloc(64, index->size*sizeof(bin)*2);
    debug("[Debug] <!> index_resize with size %ld start, *ent=%p\n", index->size*2, index->entries);
    stat(index);
    BIN_FLAG_TYPE temp, *new_occupied = malloc(index->size*2*sizeof(BIN_FLAG_TYPE));
    memcpy(new_entries, index->entries, index->size*sizeof(bin));
    memcpy(new_entries + index->size, index->entries, index->size*sizeof(bin));
    memcpy(new_occupied, index->occupied, index->size*sizeof(BIN_FLAG_TYPE));
    memcpy(new_occupied + index->size, index->occupied, index->size*sizeof(BIN_FLAG_TYPE));
    
    for(uint64_t i=0; i<index->size; i++){
        if(!index->occupied[i]) continue;
        temp = 0;
        curr = index->entries + i;
        for(int j=0; j<BIN_CAPACITY; j++){
            if(TEST_BIT(new_occupied[i], j) && HASH_LEVEL(index, curr->data[j].key))
                SET_BIT(temp, j);
        }
        //debug("bin %d: flag=%x\n", i, temp);
        new_occupied[i] &= ~temp;
        new_occupied[i + index->size] &= temp;
    }
    free(index->entries);
    index->entries = new_entries;
    free(index->occupied);
    index->occupied = new_occupied;
    index->size *= 2;
    debug("[Debug] <!> index_resize with size %ld ends\n", index->size);
    stat(index);
    return 0;
};

int index_expand_reinsert(index_sys* index){
    index_sys* new_index = index_construct(index->size*2, index->seed);
    debug("[Debug] <!> index_resize with size %ld and count %ld start, *ent=%p\n", new_index->size, index->entries, index->count);
    bin* curr;
    for(uint64_t i=0; i<index->size; i++){
        if(!index->occupied[i]) continue;
        curr = index->entries + i;
        for(int j=0; j<BIN_CAPACITY; j++){
            if(TEST_BIT(index->occupied[i], j))
                index_insert(new_index, curr->data[j].key, curr->data[j].value, MEM_TYPE);
        }
    }
    free(index->entries);
    index->entries = new_index->entries;
    free(index->occupied);
    index->occupied = new_index->occupied;
    free(new_index);
    index->size *= 2;
    debug("[Debug] <!> index_resize with size %ld ends\n", index->size);
    return 0;
};

int index_insert(index_sys* index, const uint8_t* key, const uint8_t* value, int storage_type){
    // TODO: check for duplication?
    uint64_t idx = HASH_IDX(index, key);
    // ------------------- Find/Create the bin to insert into --------------------
    if(index->occupied[idx]==FULL_FLAG){
        index_expand_reinsert(index); // suppose rehashing is done in expansion.
        idx = HASH_IDX(index, key);
    }
    bin* target = index->entries + idx;
    // ------------------- Insert into the bin referenced by 'target' --------------------
    int j=0;
    if(storage_type==MEM_TYPE){ // If data is on dram/pmem, place at front
        for(j=0; j<BIN_CAPACITY && TEST_BIT(index->occupied[idx], j);) j++;
    }
    else{ // else place at back.
        for(j=BIN_CAPACITY-1; j>=0 && TEST_BIT(index->occupied[idx], j);) j--;
    } 
    if(j==BIN_CAPACITY || j<0)
        debug("[ERROR] j should be between [0, %d], but now is %d. With flags=%2x\n", BIN_CAPACITY-1, j, index->occupied[idx]);
    *(uint64_t*)target->data[j].key = *(uint64_t*)key;
    *(uint64_t*)target->data[j].value = *(uint64_t*)value;
    // ------------------- Update metadata ---------------------
    // TODO: the following actions need to be [ATOMIC]
    SET_BIT(index->occupied[idx], j);
    index->count++;
    // [ATOMIC] END
    return 0;
};

entry* index_entry_query(index_sys* index, const uint8_t* key){
    uint64_t key_int64 = *(uint64_t*)key, i = HASH_IDX(index, key);
    bin* target = index->entries + i;
    for(int j=0; j<BIN_CAPACITY; j++)
        if(TEST_BIT(index->occupied[i], j) && *(uint64_t*)target->data[j].key == key_int64)
            return target->data+j;
    return NULL;
}

uint8_t* index_query(index_sys* index, const uint8_t* key){
    entry* item = index_entry_query(index, key);
    if(item)
        return item->value;
    return NULL;
};

int index_update(index_sys* index, const uint8_t* key, const uint8_t* value){
    entry* item = index_entry_query(index, key);
    if(item){
        // TODO: make it (explicitly) atomic? cacheline size 64 bit? 
        // TODO: consider race condition?
        *(uint64_t*)item->value = *(uint64_t*)value;
        return 0; // OK
    }
    return 1; // element not found;
};
int index_delete(index_sys* index, const uint8_t* key){
    uint64_t key_int64 = *(uint64_t*)key, i = HASH_IDX(index, key);
    bin* target = index->entries + i;
    for(int j=0; j<BIN_CAPACITY; j++)
        if(TEST_BIT(index->occupied[i], j) && *(uint64_t*)target->data[j].key==*(uint64_t*)key){
            UNSET_BIT(index->occupied[i], j);
            index->count--;
            return 0;
        }
    return 1;
};
