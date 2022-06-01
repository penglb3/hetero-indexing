#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <map>
#include <chrono>
#include <algorithm>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <cassert>
#include "hashBenchmark.hpp"
#include "common.h"
#include "art.h"


int main(int argc, char ** argv){
    HashBenchmark hm(argc, argv, "ART", "DRAM");
    std::map<std::string, double> parameters;
    const std::vector<operation_t>& load_ops = hm.load_ops;
    const std::vector<operation_t>& run_ops = hm.run_ops;
    index_sys* ind = (index_sys*) calloc(1, sizeof(index_sys));
    ind->tree = (art_tree*) calloc(1, sizeof(art_tree));
    art_tree_init(ind->tree);
    int mem_type = 0; 
    
    hm.Start("load");
    // load
    for (auto op : load_ops ){
        switch (op.op)
        {   
            case OP_INSERT: {
                art_insert_no_replace(ind->tree, (const uint8_t*)&op.key, KEY_LEN | op.info, &op.value);
                break;
            }
            default: {
                std::cout << "Unknown OP In Load : " << op.op << std::endl;
                break;
            }
        }
        hm.Step();
    }

    hm.End();
    #ifdef BUF_LEN
    printf(ANSI_COLOR_GREEN "[Hetero]" ANSI_COLOR_RESET " #INBs: %lu / %lu (%.3lf%)\n" , ind->tree->buffer_count, ind->tree->size, (double) ind->tree->buffer_count * 100 / ind->tree->size);
    #endif
    hm.Print(parameters);

    // // run 
    hm.Start("run");

    for (auto op : run_ops ){
        switch (op.op)
        {
            case OP_INSERT: {
                art_insert_no_replace(ind->tree, (const uint8_t*)&op.key, KEY_LEN | op.info?MEM_TYPE:IO_TYPE, &op.value);
                break;
            }
            case OP_READ: {
                hash_value_t read_result;
                auto error_code = art_search(ind, (const uint8_t*)&op.key, NULL, &read_result);
                // IMPORTANT!! verify the correctness
                if (read_result != op.value){
                    std::cout << "READ value wrong !! " << "Read Key : [" << op.key << "] You Read: [" << read_result << "] Expected Value is: [" << op.value << "]" << std::endl; 
                    std::cout << "You should correct this BUG!" << std::endl;
                    return -1;
                }
                break;
            }
            case OP_UPDATE: {
                art_update(ind, (const uint8_t*)&op.key, KEY_LEN, &op.value);
                break;
            }
            case OP_DELETE: {
                art_delete(ind->tree, (const uint8_t*)&op.key, KEY_LEN);
                if (hm.shouldCheck()){
                    std::cout << "Not Implemented OP[DELETE] In Run : " << op.op << std::endl;
                    exit(-1);
                }
                break;
            }
            default: {
                std::cout << "Unknown OP In Run : " << op.op << std::endl;
                break;
            }
        }
        hm.Step();
    }

    hm.End();
    if(mem_type)
        printf("[Hetero] Number of mem_types: %d\n", mem_type), mem_type = 0;
    hm.Print(parameters);
    return 0;
}
