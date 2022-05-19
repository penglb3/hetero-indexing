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
#include "Level-Hashing/level_hashing/level_hashing.h"

int main(int argc, char ** argv){
    HashBenchmark hm(argc, argv, "Level-hashing", "DRAM");
    std::map<std::string, double> parameters;
    const std::vector<operation_t>& load_ops = hm.load_ops;
    const std::vector<operation_t>& run_ops = hm.run_ops;

    level_hash* ind = level_init(17);
    int reinsert = 0; 
    
    hm.Start("load");
    // load
    for (auto op : load_ops ){
        switch (op.op)
        {   
            case OP_INSERT: {
                while(level_insert(ind, (uint8_t*)&op.key, (uint8_t*)&op.value) && reinsert < 2){
                    level_expand(ind);
                    reinsert++;
                } 
                if(reinsert == 2){
                    printf("Level insert failed for Key[%lu]\n", op.key);
                    return 2;
                }
                reinsert = 0;
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
    hm.Print(parameters);

    // // run 
    hm.Start("run");

    for (auto op : run_ops ){
        switch (op.op)
        {
            case OP_INSERT: {
                while(level_insert(ind, (uint8_t*)&op.key, (uint8_t*)&op.value) && reinsert < 2){
                    level_expand(ind);
                    reinsert++;
                } 
                if(reinsert == 2){
                    printf("Level insert failed for Key[%lu]\n", op.key);
                    return 2;
                }
                reinsert = 0;
                reinsert = 0;
                break;
            }
            case OP_READ: {
                hash_value_t* result_ptr = (hash_value_t*) level_static_query(ind, (uint8_t*)&op.key);
                // IMPORTANT!! verify the correctness
                if (!result_ptr){
                    printf("result when READing %lu returned NULL, expected %lu. Abort.\n", op.key, op.value);
                    return 1;
                }
                if (*result_ptr != op.value){
                    std::cout << "READ value wrong !! " << "Read Key : [" << op.key << "] You Read: [" << *result_ptr << "] Expected Value is: [" << op.value << "]" << std::endl; 
                    std::cout << "You should correct this BUG!" << std::endl;
                    return -1;
                }
                break;
            }
            case OP_UPDATE: {
                level_update(ind, (uint8_t*)&op.key, (uint8_t*)&op.value);
                break;
            }
            case OP_DELETE: {
                level_delete(ind, (uint8_t*)&op.key);
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
    hm.Print(parameters);
    return 0;
}
