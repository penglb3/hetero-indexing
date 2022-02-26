#include"hetero.h"
#include<unistd.h>
#include<stdio.h>
#include<time.h>

// toy example
int main(int argc, char* argv[]){
    uint64_t size = 1<<14, num_entries = 500000, seed = 0;
    debug = do_nothing;
    hash_expand = hash_expand_copy;
    int c;
    while((c = getopt(argc, argv, "s:n:di:r"))!=-1)
        switch(c){
            case 'i': seed = atoi(optarg); break;
            case 's': size = 1<<atoi(optarg);break;
            case 'n': num_entries = atoi(optarg);break;
            case 'd': debug = printf;break;
            case 'r': hash_expand = hash_expand_reinsert; break;
            default : abort();
        }
    index_sys* index = index_construct(size, seed);
    if(!index){
        printf("init failed: %d\n",c);
        return -1;
    }
    printf("Initial size = %ldx%d, # of samples = %ld, seed: %ld\n", index->hash->size, BIN_CAPACITY, num_entries, index->hash->seed);
    uint64_t key, val;
    uint8_t* val_ret;
    // insertion
    printf("Insert Test: ");
    clock_t start = clock(),finish;
    for(uint64_t key=0; key<num_entries; key++){
        size = index->hash->count;
        c = index_insert(index, (const uint8_t*)&key, (const uint8_t*)&key, MEM_TYPE);
        if(index->hash->count > size && size < BIN_CAPACITY*index->hash->size*0.95+10 && size >= BIN_CAPACITY*index->hash->size*0.95)
            printf("[Note] Key #%ld inserted into hash table (%ld)\n", key, index->hash->count);
        if(c){
            printf("!! insertion of key-value pair <%ld,%ld> failed. !!\n", key, key);
            return 1;
        }
    }
    finish = clock() - start;
    printf("Finished in %.3lf s. \n# of entries in indexing system:%ld(%ld+%ld), size of hash table:%ldx%d\n", 
        (double)finish / CLOCKS_PER_SEC, index->hash->count + index->tree->size, index->hash->count, index->tree->size, index->hash->size, BIN_CAPACITY);
    // query
    printf("Query Test: ");
    start = clock();
    for(uint64_t i=0; i<num_entries; i++){
        key = (uint64_t)&i;
        val_ret = index_query(index, (const uint8_t*)key);
        if(val_ret==NULL || (*(uint64_t*)(val_ret)) != i){
            printf("!! query of key %ld failed:", i);
            printf("val_ret %s, ", val_ret?"non null":"null");
            printf("result %s)\n", (*(uint64_t*)(val_ret)) != i?"not equal":"equal");
            return 2;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lfs.\n", (double)finish / CLOCKS_PER_SEC);
    // update
    printf("Update Test: ");
    start = clock();
    for(uint64_t i=0; i<num_entries; i++){
        key = (uint64_t)&i;
        val = 2*i;
        c = index_update(index, (const uint8_t*)key, (const uint8_t*)&val);
        val_ret = index_query(index, (const uint8_t*)key);
        if(c || val_ret==NULL || (*(uint64_t*)(val_ret)) != val){
            printf("!! update of key %ld failed (", i);
            printf("update %s, ", c?"failed":"success");
            printf("val_ret %s, ", val_ret?"non null":"null");
            printf("result %s)\n", (*(uint64_t*)(val_ret)) != val?"not equal":"equal");
            return 3;
        }
    }
    finish = clock() - start;
    printf("Passed in %.3lfs.\n", (double)finish / CLOCKS_PER_SEC);
    // removal
    printf("Remove Test: ");
    start = clock();
    for(uint64_t i=0; i<num_entries; i++){
        c = index_delete(index, (const uint8_t*)&i);
        if(c){
            printf("!! removal of key %ld failed !!\n", i);
            return 4;
        }
    }
    finish = clock() - start;
    if(index->hash->count==0 && index->tree->size==0)
        printf("Passed in %.3lfs.\n", (double)finish / CLOCKS_PER_SEC);
    else
        printf("Buggy: #(hash) = %ld, #(tree) = %ld\n", index->hash->count, index->tree->size);
    index_destruct(index);
    return 0;
}