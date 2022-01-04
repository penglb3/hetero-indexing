#include"hetero.h"
#include<unistd.h>
#include<stdio.h>
// toy example
int main(int argc, char* argv[]){
    uint64_t size = 1<<3, num_entries = 500, seed = 0;
    debug = do_nothing;
    int c;
    while((c = getopt(argc, argv, "s:n:dr:"))!=-1)
        switch(c){
            case 'r': seed = atoi(optarg); break;
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
    printf("size = %ld, # of samples = %ld, seed: %ld\n", size, num_entries, index->seed);
    uint64_t key, val;
    uint8_t* val_ret;
    // insertion
    for(uint64_t key=0; key<num_entries; key++){
        c = index_insert(index, (const uint8_t*)&key, (const uint8_t*)&key, 1);
        if(c){
            printf("!! insertion of key-value pair <%ld,%ld> failed. !!\n", key, key);
            return 1;
        }
    }
    printf("Insertion all done. \n# of entries in indexing system:%ld, size of it:%ld\n", index->count, index->size);
    // query
    for(uint64_t i=0; i<num_entries; i++){
        key = (uint64_t)&i;
        val_ret = index_query(index, (const uint8_t*)key);
        if(val_ret==NULL || (*(uint64_t*)(val_ret)) != i){
            printf("query of key %ld failed\n", i);
            return 2;
        }
    }
    printf("Query all OK.\n");
    // update
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
    printf("Update all OK.\n");
    // removal
    for(uint64_t i=0; i<num_entries; i++){
        c = index_delete(index, (const uint8_t*)&i);
        if(c){
            printf("!! removal of key %ld failed !!\n", i);
            return 4;
        }
    }
    if(index->count==0)
        printf("Remove all OK.\n");
    else
        printf("Removal is buggy: # of active entries in indexing system:%ld\n", index->count);
    printf("Test finished\n");
    index_destruct(index);
    return 0;
}