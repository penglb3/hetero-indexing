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


int main(int argc, char ** argv){
    HashBenchmark hm(argc, argv, "unordered_map", "DRAM");
    std::map<std::string, double> parameters;
    const std::vector<operation_t>& load_ops = hm.load_ops;
    const std::vector<operation_t>& run_ops = hm.run_ops;
    std::unordered_map<hash_key_t, hash_value_t> test_map;
    
    hm.Start("load");
    // load
    for (auto op : load_ops ){
        switch (op.op)
        {   
            case OP_INSERT: {
                test_map.insert({op.key, op.value});
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
                test_map.insert({op.key, op.value});
                break;
            }
            case OP_READ: {
                auto read_result = test_map[op.key];
                // IMPORTANT!! verify the correctness
                if (read_result != op.value){
                    std::cout << "READ value wrong !! " << "Read Key : [" << op.key << "] You Read: [" << read_result << "] Expected Value is: [" << op.value << "]" << std::endl; 
                    std::cout << "You should correct this BUG!" << std::endl;
                    return -1;
                }
                break;
            }
            case OP_UPDATE: {
                test_map[op.key] = op.value;
                // std::cout << "Not Implemented OP[UPDATE] In Run : " << op.op << std::endl;
                break;
            }
            case OP_DELETE: {
                test_map.erase(op.key);
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
