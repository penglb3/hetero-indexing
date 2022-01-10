#include"hetero.h"
#include<unistd.h>
#include<stdio.h>
#include<time.h>


// toy example
int main(int argc, char* argv[]){
    uint64_t size = 1<<14, num_entries = 500000, seed = 0;
    debug = do_nothing;
    int c;
    while((c = getopt(argc, argv, "s:n:di:"))!=-1)
        switch(c){
            case 'i': seed = atoi(optarg); break;
            case 's': size = 1<<atoi(optarg);break;
            case 'n': num_entries = atoi(optarg);break;
            case 'd': debug = printf;break;
            default : abort();
        }
    index_sys* index = index_construct(size, seed);
    if(!index){
        printf("init failed: %d\n",c);
        return -1;
    }
    printf("Initial size = %ld, # of samples = %ld, seed: %ld\n", size, num_entries, index->seed);
    uint64_t key, val;
    uint8_t* val_ret;
    // insertion
    printf("Insert Test: ");
    clock_t start = clock(),finish;
    for(uint64_t key=0; key<num_entries; key++){
        c = index_insert(index, (const uint8_t*)&key, (const uint8_t*)&key, 1);
        if(c){
            printf("!! insertion of key-value pair <%ld,%ld> failed. !!\n", key, key);
            return 1;
        }
    }
    finish = clock() - start;
    printf("Finished in %.3lf s. \n# of entries in indexing system:%ld, size of it:%ld\n", (double)finish / CLOCKS_PER_SEC, index->count, index->size);
    // query
    printf("Query Test: ");
    start = clock();
    for(uint64_t i=0; i<num_entries; i++){
        key = (uint64_t)&i;
        val_ret = index_query(index, (const uint8_t*)key);
        if(val_ret==NULL || (*(uint64_t*)(val_ret)) != i){
            printf("query of key %ld failed\n", i);
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
            printf("!! update of key %ld failed !!\n", i);
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
    if(index->count==0)
        printf("Passed in %.3lfs.\n", (double)finish / CLOCKS_PER_SEC);
    else
        printf("Buggy: # of active entries in indexing system = %ld\n", index->count);
    index_destruct(index);
    return 0;
}