#include "hetero.h"
#include "atomic.h"
#include "queue.h"
#include <stdio.h>
// -------------- INDEX SYS --------------

index_sys* index_construct(uint64_t hash_size, uint64_t seed){
    index_sys* index = calloc(1, sizeof(index_sys));
    if(!index) 
        return NULL;
    index->hash = hash_construct(hash_size, seed);
    index->tree = calloc(1, sizeof(art_tree));
    index->has_special_key[0] = 0;
    index->has_special_key[1] = 0;
    if(!index->hash || !index->tree) 
        return NULL;
    index->cm = countmin_construct(hash_size, CM_DEPTH, index->hash->seed);
    if(!index->cm)
        return NULL;
    return art_tree_init(index->tree) ? NULL : index;
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
    if(IS_SPECIAL_KEY(key)){
        *(uint64_t*)(index->special_key_val + *(uint64_t*)key) = *(uint64_t*)value;
        *(index->has_special_key + *(uint64_t*)key) = OCCUPIED_FLAG;
        return 0;
    }
    if(storage_type == MEM_TYPE){
        REINSERT:;
        status = hash_insert(index, key, value);
        if(status == 0){
            return 0;
        }
        if(index->hash->count * MAX_TREE_HASH_RATIO < index->tree->size){
            hash_expand(& index->hash);
            countmin_clear(index->cm);
            index_compact(index, 0.8);
            goto REINSERT;
        }

    }
    return art_insert_no_replace(index->tree, key, storage_type | KEY_LEN, (void*)value) != NULL;
}

int index_query(index_sys* index, const uint8_t* key, uint64_t* query_result){
    if(IS_SPECIAL_KEY(key)){
        if(*(index->has_special_key + *(uint64_t*)key)){
            *query_result = *((uint64_t*)index->special_key_val[*(uint64_t*)key]);
            return 0;
        }
        else
            return 1;
    }
    uint64_t h[2];
    int freq;
    if(hash_query(index, key, h, query_result)){
        #ifdef USE_CM
        countmin_inc_explicit(index->cm, key, KEY_LEN, h);
        #endif
        return 0;
    }
    if(art_search(index, key, h, query_result)){
        #ifdef USE_CM
        countmin_inc_explicit(index->cm, key, KEY_LEN, h);
        #endif
        return 0;
    }
    return 1;
}

int index_update(index_sys* index, const uint8_t* key, const uint8_t* value){
    if(IS_SPECIAL_KEY(key)){
        *(uint64_t*)(index->special_key_val + *(uint64_t*)key) = *(uint64_t*)value;
        *(index->has_special_key + *(uint64_t*)key) = OCCUPIED_FLAG;
        return 0;
    }
    int freq;
    int status = hash_update(index, key, value, &freq);
    if(status == ELEMENT_NOT_FOUND){
        return art_update(index, key, freq, (void*)value) == NULL;
    }
    return 0;
}

int index_delete(index_sys* index, const uint8_t* key){
    if(IS_SPECIAL_KEY(key)){
        uint8_t s = 1;
        return !atomic_compare_exchange_strong(index->has_special_key + *(uint64_t*)key, &s, EMPTY_FLAG);
    }
    uint64_t h[2];
    int found = hash_delete(index, key, h);
    if(!found){
        return art_delete(index->tree, key, KEY_LEN) == NULL;
    }
    if( load_factor(index->hash) < COMPACT_START_LOAD_FACTOR && index->tree->buffer_count )
        index_compact(index, MAX_COMPACT_LOAD_FACTOR);
    return 0;
}

static inline int is_leaf(art_node* n) {
    return (uintptr_t)n & 1;
}

int index_compact(index_sys* index, double max_load_factor){
    debug("\n[Debug] Refill starts with load factor %.3lf, [H]%lu+[TB]%lu: ", load_factor(index->hash), index->hash->count, index->tree->buffer_count);
    queue* q = queue_construct(index->hash->size >> 4);
    art_node* n = index->tree->root, **children, *c;
    int error, consecutive_failure = 0, cnt = index->hash->count, n_children, total = index->tree->size + index->hash->count;
    queue_push(q, n);
    #if defined(COMPACT) && defined(BUF_LEN)
    while(!queue_empty(q)){
        n = queue_pop(q);
        if(is_leaf(n)) continue; // IS LEAF
        for(int i = 0; i < BUF_LEN; i++){
            if(!IS_SPECIAL_KEY(n->buffer[i].key)){
                if(consecutive_failure < 3 && load_factor(index->hash) < max_load_factor){
                    error = hash_insert_nocheck(index->hash, n->buffer[i].key, n->buffer[i].value);
                    if (!error){
                        consecutive_failure = 0;
                        goto CLEAR_BUFFER;
                    }
                    consecutive_failure++;
                }
                error = art_insert_no_replace(index->tree, n->buffer[i].key, KEY_LEN | MEM_TYPE, n->buffer[i].value) != NULL;
                CLEAR_BUFFER:;
                if (!error){
                    *(uint64_t*)n->buffer[i].key = 0;
                    index->tree->buffer_count--;
                    index->tree->size--;
                }
            }
        }
        switch (n->type) {
            case NODE4:
                children = ((art_node4*)n)->children; n_children = 4; break;
            case NODE16:
                children = ((art_node16*)n)->children; n_children = 16; break;
            case NODE48:
                children = ((art_node48*)n)->children; n_children = 48; break;
            case NODE256:
                children = ((art_node256*)n)->children; n_children = 256; break;
            default:
                abort();
        }
        if(!index->tree->buffer_count)
            break;
        for(int k = 0; k < n_children; k++){
            c = children[k];
            if(c && !is_leaf(c))
                queue_push(q, c);
        }
    }
    #endif // COMPACT
    queue_destruct(q);
    debug("[CF]%d [Hash Fill]%d\n", consecutive_failure, index->hash->count - cnt);
    return 0;
}

int index_expand(index_sys* index){
    hash_expand(&index->hash);
    return index_compact(index, 0.8);
}

uint64_t index_size(index_sys* index){
    return index->hash->count + index->tree->size + index->has_special_key[0] + index->has_special_key[1];
}
