#include "hetero.h"

int do_nothing(const char* format, ...){return 0;};

uint64_t idx_hash(index_sys* index, const uint8_t* data){
    return MurmurHash3_x64_128(data, KEY_LEN, index->seed) % index->size;
}

void index_destruct(index_sys* index){
    free(index->entries);
    free(index);
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
    if(index->entries==NULL) 
        return NULL;
    // TODO: 1. really need? 2. Any quicker way?
    memset(index->entries, 0, size*sizeof(bin)); 
    index->size = size;
    index->count = 0;
    return index;
}

int index_resize(index_sys* index, uint64_t new_size){
    index_sys* new_index = index_construct(new_size, index->seed);
    debug("[Debug] <!> index_resize with size %ld start, *ent=%p\n", new_size, index->entries);
    bin* curr, *temp;
    for(int i=0; i<index->size; i++){
        while(index->entries[i].count==0) 
            i++;
        curr = index->entries + i;
        debug("[Debug] <!> dumping bin#%d:", i);
        int t = 0;
        while(curr){
            debug("c#%d:\n", t++);
            for(int j=0; j<BIN_CAPACITY; j++){ // REHASH
                if(curr->status[j])
                    index_insert(new_index, curr->data[j].key, curr->data[j].value, curr->status[j]);
            }
            temp = curr;
            curr = curr->next;
            if(temp!=index->entries+i)
                free(temp);
        }
    }
    free(index->entries);
    index->entries = new_index->entries;
    index->size = new_size;
    free(new_index);
    debug("[Debug] <!> index_resize with size %ld ends\n", new_size);
    return 0;
};

int index_insert(index_sys* index, const uint8_t* key, const uint8_t* value, int storage_type){
    // TODO: check for duplication?

    bin* target = index->entries + idx_hash(index, key), *temp;
    int i;
    // ------------------- Find/Create the bin to insert into --------------------
    // Trying to find an unfull bin
REINSERT:
    //debug("[Debug] Inserting key %ld to bin#%ld", *(uint64_t*)key, target - index->entries);
    for(i=0; target->count==BIN_CAPACITY && target->next && i<MAX_CHAIN_LEN; i++) 
        target = target->next; //!pointer chasing here, avoid with sth like 2d-indexing in CCEH?
    //debug(" ..to chain#%d, count=%d\n", i, target->count);
    // If reached maximum chain length
    if(target->count==BIN_CAPACITY){
        if(i==MAX_CHAIN_LEN-1){
            index_expand(index); // suppose rehashing is done in expansion.
            target = index->entries + idx_hash(index, key);
            goto REINSERT;
        }
        else {
            debug("[Debug] appending new bin to bin#%ld, chain#%d->%d\n", idx_hash(index, key), i, i+1);
            temp = aligned_alloc(64, sizeof(bin));
            memset(temp, 0, 16);
            // TODO: [ATOMIC]
            target->next = temp;
            // [ATOMIC] ENDS
            target = target->next;
        }
    }

    // ------------------- Insert into the bin referenced by 'target' --------------------
    int j=0;
    if(storage_type==MEM_TYPE){ // If data is on dram/pmem, place at front
        for(j=0; j<BIN_CAPACITY && target->status[j];) j++;
    }
    else{ // else place at back.
        for(j=BIN_CAPACITY-1; j>=0 && target->status[j];) j--;
    } 
    if(j==BIN_CAPACITY || j<0)
        debug("[ERROR] j should be between [0, %d], but now is %d. At chain#%d with count=%d\n", BIN_CAPACITY-1, j, i, target->count);
    *(uint64_t*)target->data[j].key = *(uint64_t*)key;
    *(uint64_t*)target->data[j].value = *(uint64_t*)value;
    // ------------------- Update metadata ---------------------
    // TODO: the following actions need to be [ATOMIC]
    target->status[j] = storage_type;
    target->count++;
    index->count++;
    // [ATOMIC] END
    return 0;
};

entry* index_entry_query(index_sys* index, const uint8_t* key){
    uint64_t key_int64 = *(uint64_t*)key;
    bin* target = index->entries + idx_hash(index, key);
    while(target && target->count) {
        for(int j=0; j<BIN_CAPACITY; j++)
            if(target->status[j] && *(uint64_t*)target->data[j].key == key_int64)
                return target->data+j;
        target = target->next; //!pointer chasing here, avoid with sth like 2d-indexing in CCEH?
    }
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
        *(uint64_t*)item->value = *(uint64_t*)value;
        return 0; // OK
    }
    return 1; // element not found;
};
int index_delete(index_sys* index, const uint8_t* key){
    uint64_t key_int64 = *(uint64_t*)key;
    bin* target = index->entries + idx_hash(index, key), *prev = NULL;
    for(int i=0; i<MAX_CHAIN_LEN; i++) {
        for(int j=0; j<BIN_CAPACITY; j++)
            if(target->status[j] && *(uint64_t*)target->data[j].key == key_int64){
                target->status[j] = 0;
                if(target->count==1 && prev){
                    prev->next = target->next;
                    free(target);
                }
                else 
                    target->count--;
                index->count--;
                // if(index->count <= SHRINK_THRESHOLD * index->size * BIN_CAPACITY * MAX_CHAIN_LEN / 2)
                //     index_shrink(index);
                return 0;
            }
        if(target->next)
            prev = target, target = target->next; //!pointer chasing here, avoid with sth like 2d-indexing in CCEH?
        else 
            return 1;
    }
    return 1;
};
