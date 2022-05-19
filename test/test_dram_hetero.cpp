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
#include "hetero.h"

int main(int argc, char ** argv){
    HashBenchmark hm(argc, argv, "hetero-indexing", "DRAM");
    std::map<std::string, double> parameters;
    const std::vector<operation_t>& load_ops = hm.load_ops;
    const std::vector<operation_t>& run_ops = hm.run_ops;
    int s = __builtin_ctz(load_ops.size()/1000000);
    s = 15 + (s >= 2? s: 2);
    index_sys* ind = index_construct(1<<s, 0);
    if(!ind){
        printf("Index init failed. Abort.\n");
        abort();
    }
    int mem_type = 0; 
    
    hm.Start("load");
    // load
    for (auto op : load_ops ){
        switch (op.op)
        {   
            case OP_INSERT: {
                index_insert(ind, (const uint8_t*)&op.key, (const uint8_t*)&op.value, op.info?MEM_TYPE:IO_TYPE);
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

    // run 
    hm.Start("run");

    for (auto op : run_ops ){
        switch (op.op)
        {
            case OP_INSERT: {
                index_insert(ind, (const uint8_t*)&op.key, (const uint8_t*)&op.value, MEM_TYPE);
                break;
            }
            case OP_READ: {
                hash_value_t read_result;
                auto error_code = index_query(ind, (const uint8_t*)&op.key, &read_result);
                // IMPORTANT!! verify the correctness
                if (read_result != op.value){
                    std::cout << "READ value wrong !! " << "Read Key : [" << op.key << "] You Read: [" << read_result << "] Expected Value is: [" << op.value << "]" << std::endl; 
                    std::cout << "You should correct this BUG!" << std::endl;
                    return -1;
                }
                break;
            }
            case OP_UPDATE: {
                index_update(ind, (const uint8_t*)&op.key, (const uint8_t*)&op.value);
                break;
            }
            case OP_DELETE: {
                index_delete(ind, (const uint8_t*)&op.key);
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
