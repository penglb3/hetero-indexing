#include"hetero.h"
#include<unistd.h>
#include<stdio.h>
// toy example
int main(int argc, char* argv[]){
    uint64_t size = 1<<15, num_entries = size*5;
    int c;
    while((c = getopt(argc, argv, "s:n:"))!=-1)
        switch(c){
            case 's': size = 1<<atoi(optarg);break;
            case 'n': num_entries = atoi(optarg);break;
            default : abort();
        }
    
    index_sys* index;
    c = index_sys_init(index, size);
    if(c){
        printf("init failed: %d\n",c);
        return -1;
    }
    printf("size of indexing system:%d\n", index->size);
    uint8_t key[KEY_LEN], val[VAL_LEN];
    uint8_t* val_ret;
    // creation
    for(uint64_t i=0; i<num_entries; i++){
        sprintf(key, "%d", i);
        c = create(index, key, key, 0);
        if(c){
            printf("creation of key-value pair <%s,%s> failed\n", key, key);
            return 1;
        }
    }
    printf("size of indexing system:%d\n", index->size);
    // query
    for(uint64_t i=0; i<num_entries; i++){
        sprintf(key, "%d", i);
        val_ret = query(index, key);
        if(val_ret || strcmp(val_ret, key)){
            printf("query of key %d failed\n", i);
            return 2;
        }
    }
    // update
    for(uint64_t i=0; i<num_entries; i++){
        sprintf(key, "%d", i);
        sprintf(val, "%d", i*2);
        c = update(index, key, val);
        if(c){
            printf("update of key %d failed\n", i);
            return 3;
        }
    }
    // removal
    for(uint64_t i=0; i<num_entries; i++){
        sprintf(key, "%d", i);
        c = remove(index, key);
        if(c){
            printf("removal of key %d failed\n", i);
            return 4;
        }
    }
    printf("# of active entries in indexing system:%d\n", index->count);
    printf("test successful\n");
    return 0;
}