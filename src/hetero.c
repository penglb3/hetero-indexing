#include "hetero.h"

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
    if(*(uint64_t*)key==433412)
        debug("433412, fuck!\n");
    if(*(uint64_t*)key == 0){
        atomic_store(& index->has_zero_key, 1);
        atomic_store((uint64_t*)index->val_for_zero, *(uint64_t*)value);
        return 0;
    }
    if(storage_type == MEM_TYPE){
        status = hash_insert(index, key, value);
        if(status == 0){
            return 0;
        }
        // if(status == HASH_BIN_FULL){
        //     hash_expand(index->hash);
        //     goto REINSERT;
        // }
    }
    return art_insert_no_replace(index->tree, key, storage_type | KEY_LEN, (void*)value) != NULL;
}

uint8_t* index_query(index_sys* index, const uint8_t* key){
    if(*(uint64_t*)key == 0){
        if(atomic_load(& index->has_zero_key)){
            return index->val_for_zero;
        }
        else
            return NULL;
    }
    int cnt = countmin_inc(index->cm, (const void*)key, KEY_LEN);
    uint8_t* result = hash_query(index->hash, key);
    if(result) 
        return result;
    return art_search(index->tree, key, KEY_LEN);
}

int index_update(index_sys* index, const uint8_t* key, const uint8_t* value){
    if(*(uint64_t*)key == 0){
        atomic_store(& index->has_zero_key, 1);
        atomic_store((uint64_t*)index->val_for_zero, *(uint64_t*)value);
        return 0;
    }
    countmin_inc(index->cm, (const void*)key, KEY_LEN);
    int status = hash_update(index, key, value);
    if(status == ELEMENT_NOT_FOUND){
        return art_update(index->tree, key, KEY_LEN, (void*)value) == NULL;
    }
    return 0;
}

int index_delete(index_sys* index, const uint8_t* key){
    if(*(uint64_t*)key == 0){
        uint8_t s = 1;
        return !atomic_compare_exchange_strong(& index->has_zero_key, &s, 0);
    }
    int status = hash_delete(index->hash, key);
    if(status){
        return art_delete(index->tree, key, KEY_LEN) == NULL;
    }
    return 0;
}

uint64_t index_size(index_sys* index){
    return index->hash->count + index->tree->size + index->has_zero_key;
}