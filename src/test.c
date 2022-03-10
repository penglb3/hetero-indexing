#include <getopt.h>
#include <stdio.h>
#include <time.h>
#include "hetero.h"

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
    debug("#(ENTRY)  : BEFORE     -> AFTER\n");
    for(int i=0; i<=BIN_CAPACITY; i++)
        debug("%-10d: %-10d -> %d\n", i, count[i], count[i+BIN_CAPACITY+1]), sum += count[i] - count[i+BIN_CAPACITY+1];
    if(sum)
        debug("[WARNING] before - after = %ld, which is not 0!\n", sum);
    debug("\n");
    memset(count, 0, sizeof(uint64_t)*(BIN_CAPACITY*2+2));
}


// toy example
int main(int argc, char* argv[]){
    // ---------------------------------- Initialize ----------------------------------
    uint64_t size = 1<<17, num_entries = 2000000;
    uint32_t seed = 0;
    int error, status;
    uint64_t result_value;
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
    printf("Passed in %.3lf s. # of entries in indexing system:%lu([H]%lu+[T]%lu(%lu)+[F]%d)\n", 
        (double)finish / CLOCKS_PER_SEC, index_size(index), index->hash->count, index->tree->size, index->tree->buffer_count, index->has_special_key[0]+index->has_special_key[1]);

    // ---------------------------------- Expand ----------------------------------
    printf("Expansion Test (%s): ", hash_expand==hash_expand_copy?"copy":"reinsert");
    if(debug != do_nothing){
        log_stat(index->hash, count);
    }
    start = clock();
    hash_expand(&index->hash);
    finish = clock() - start;
    printf("%.3lfs.\n", (double)finish / CLOCKS_PER_SEC);
    if(debug != do_nothing){
        debug("[Debug] <!> hash_expand_copy with size %ld ends\n", index->hash->size);
        log_stat(index->hash, count+BIN_CAPACITY+1);
        show_stat(index->hash);
    }
    // ---------------------------------- Query ----------------------------------
    printf("Query Test: ");
    start = clock();
    for(uint64_t i=0; i<num_entries; i++){
        key = (uint64_t)&i;
        status = index_query(index, (const uint8_t*)key, &result_value);
        if(status || result_value != i){
            printf("!! query of key %lu failed ", i);
            printf("(return code %d", status);
            if(!status) printf(", query result %lu[%sequal])\n", result_value, result_value != i?"not ":"");
            else printf(")\n");
            error = 2;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lfs. # of entries in indexing system:%lu([H]%lu+[T]%lu(%lu)+[F]%d)\n"
        , (double)finish / CLOCKS_PER_SEC, index_size(index), index->hash->count, index->tree->size, index->tree->buffer_count, index->has_special_key[0]+index->has_special_key[1]);
    // ---------------------------------- Update ----------------------------------
    printf("Update Test: ");
    start = clock();
    for(uint64_t i=0; i<num_entries; i++){
        key = (uint64_t)&i;
        val = 2*i + 1;
        error = index_update(index, (const uint8_t*)key, (const uint8_t*)&val);
        val = i;
        status = index_query(index, (const uint8_t*)key, &result_value);
        if(status || result_value != 2*i+1){
            printf("!! update of key %lu failed ", i);
            printf("(return code %d", status);
            if(!status) printf(", query result %lu[%sequal])\n", result_value, result_value != 2*i+1?"not ":"");
            else printf(")\n");
            error = 2;
            goto EXIT;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lfs. # of entries in indexing system:%lu([H]%lu+[T]%lu(%lu)+[F]%d)\n"
        , (double)finish / CLOCKS_PER_SEC, index_size(index), index->hash->count, index->tree->size, index->tree->buffer_count, index->has_special_key[0]+index->has_special_key[1]);
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
        printf("Passed Clean in %.3lfs. Tree buffer count = %lx\n", (double)finish / CLOCKS_PER_SEC, index->tree->buffer_count);
    else
        printf("%.3lfs, Buggy: #(hash) = %lu, #(tree) = %lu\n", (double)finish / CLOCKS_PER_SEC, index->hash->count, index->tree->size);
    // ---------------------------------- Free up resources ----------------------------------
EXIT:;
    index_destruct(index);
    return error;
}