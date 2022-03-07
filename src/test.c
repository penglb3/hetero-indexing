#include <getopt.h>
#include <stdio.h>
#include <time.h>
#include "hetero.h"

// toy example
int main(int argc, char* argv[]){
    // ---------------------------------- Initialize ----------------------------------
    uint64_t size = 1<<17, num_entries = 2000000;
    uint32_t seed = 0;
    int error;
    double load_factor = -1;
    debug = do_nothing;
    hash_expand = hash_expand_copy;
    while((error = getopt(argc, argv, "s:n:di:rf:"))!=-1)
        switch(error){
            case 'i': seed = atoi(optarg); break;
            case 's': size = 1<<atoi(optarg);break;
            case 'n': num_entries = atoll(optarg);break;
            case 'd': debug = printf;break;
            case 'r': hash_expand = hash_expand_reinsert; break;
            case 'f': load_factor = atof(optarg); break;
            default : abort();
        }
    index_sys* index = index_construct(size, seed); // 5247926114463431242
    if(!index){
        printf("init failed: %d\n",error);
        return -1;
    }
    printf("Initial size = %lux%d, # of samples = %lu, seed: %u\n", index->hash->size, BIN_CAPACITY, num_entries, index->hash->seed);
    uint64_t key, val;
    const uint8_t* val_ret;

    // ---------------------------------- Insert ----------------------------------
    printf("Insert Test: ");
    clock_t start = clock(),finish;
    for(uint64_t key=0; key<num_entries; key++){
        size = index->hash->count;
        error = index_insert(index, (const uint8_t*)&key, (const uint8_t*)&key, MEM_TYPE);
        if(index->hash->count > size && load_factor >= 0 && size < BIN_CAPACITY*index->hash->size*load_factor+10 && size >= BIN_CAPACITY*index->hash->size*load_factor)
            printf("[Note] Key #%lu inserted into hash table (%lu)\n", key, index->hash->count);
        if(error){
            printf("!! insertion of key-value pair <%lu,%lu> failed. !!\n", key, key);
            error = 1;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lf s. # of entries in indexing system:%lu([H]%lu+[T]%lu+[F]%d), size of hash table:%lux%d\n", 
        (double)finish / CLOCKS_PER_SEC, index_size(index), index->hash->count, index->tree->size, index->has_special_key[0]+index->has_special_key[1], index->hash->size, BIN_CAPACITY);

    // ---------------------------------- Expand ----------------------------------
    printf("Expansion Test (%s): ", hash_expand==hash_expand_copy?"copy":"reinsert");
    start = clock();
    hash_expand(&index->hash);
    finish = clock() - start;
    printf("%.3lfs.\n", (double)finish / CLOCKS_PER_SEC);

    // ---------------------------------- Query ----------------------------------
    printf("Query Test: ");
    start = clock();
    for(uint64_t i=0; i<num_entries; i++){
        key = (uint64_t)&i;
        val_ret = index_query(index, (const uint8_t*)key);
        if(val_ret==NULL || (*(uint64_t*)(val_ret)) != i){
            printf("!! query of key %lu failed ", i);
            printf("(val_ret %p[%s]", val_ret, val_ret?"non null":"null");
            if(val_ret) printf(", result %s)\n", (*(uint64_t*)(val_ret)) != i?"not equal":"equal");
            else printf(")\n");
            error = 2;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lfs.\n", (double)finish / CLOCKS_PER_SEC);

    // ---------------------------------- Update ----------------------------------
    printf("Update Test: ");
    start = clock();
    for(uint64_t i=0; i<num_entries; i++){
        key = (uint64_t)&i;
        val = 2*i + 1;
        error = index_update(index, (const uint8_t*)key, (const uint8_t*)&val);
        val = i;
        val_ret = index_query(index, (const uint8_t*)key);
        if(error || val_ret==NULL || (*(uint64_t*)(val_ret)) != 2*i+1){
            printf("!! update of key %lu failed (", i);
            printf("return code %d, ", error);
            printf("val_ret %s", val_ret?"non null":"null");
            if(val_ret) printf(", result %sequal)\n", (*(uint64_t*)(val_ret)) == 2*i+1?"not ":"");
            else printf(")\n");
            error = 3;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lfs. # of entries in indexing system:%lu([H]%lu+[T]%lu+[F]%d)\n"
        , (double)finish / CLOCKS_PER_SEC, index_size(index), index->hash->count, index->tree->size, index->has_special_key[0]+index->has_special_key[1]);
    // ---------------------------------- Remove ----------------------------------
    printf("Remove Test: ");
    start = clock();
    for(uint64_t i=0; i<num_entries; i++){
        size = index_size(index);
        error = index_delete(index, (const uint8_t*)&i);
        if(error || size == index_size(index)){
            printf("!! removal of key %lu is faulty (return code %d, size remains %lu)\n", i, error, size);
            error = 4;
            goto EXIT;
        }
    }
    finish = clock() - start;
    if(index_size(index)==0)
        printf("Passed Clean in %.3lfs. Tree buffer count = %lu\n", (double)finish / CLOCKS_PER_SEC, index->tree->buffer_count);
    else
        printf("%.3lfs, Buggy: #(hash) = %lu, #(tree) = %lu\n", (double)finish / CLOCKS_PER_SEC, index->hash->count, index->tree->size);
    // ---------------------------------- Free up resources ----------------------------------
EXIT:;
    index_destruct(index);
    return error;
}