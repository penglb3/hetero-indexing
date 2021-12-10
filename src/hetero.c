#include "hetero.h"

int index_sys_init(index_sys* index, uint64_t size){
    index = malloc(sizeof(index_sys));
    if(!index) 
        return 1;
    index->entries = malloc(size*sizeof(entry));
    if(!index->entries) 
        return 2;
    index->size = size;
    return 0;
}

// TODO: implement some different hash variants as a warmup? See how hashing can cooperate with trees.

// --------------------- CUCKOO -------------------------------



// --------------------- CUCKOO END ---------------------------