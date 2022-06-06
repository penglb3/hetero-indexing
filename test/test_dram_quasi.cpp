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
#include "experimental/quasi-dynamic.h"

int main(int argc, char ** argv){
    HashBenchmark hm(argc, argv, "QD-[static]", "DRAM");
    std::map<std::string, double> parameters;
    const std::vector<operation_t>& load_ops = hm.load_ops;
    const std::vector<operation_t>& run_ops = hm.run_ops;
    qd_hash* qd = qd_hash_init(16, 1<<15, 768430);
    int error = 0; 
    
    hm.Start("load");
    // load
    for (auto op : load_ops ){
        switch (op.op)
        {   
            case OP_INSERT: {
                error = qd_hash_insert(qd, op.key, op.value);
                if(error){
                    printf(" Insert key %lx failed (%.3lf@step%lu) (Bin at head? %d. At tail? %d.)\n", op.key, qd_load_factor(qd), hm.step_count, error & 2, error & 4);
                    abort();
                }
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
    printf("[QD] Load factor = %.3lf\n", qd_load_factor(qd));
    hm.Print(parameters);

    // // run 
    hm.Start("run");

    for (auto op : run_ops ){
        switch (op.op)
        {
            case OP_INSERT: {
                error = qd_hash_insert(qd, op.key, op.value);
                if(error){
                    printf(" Insert key %lx failed\n", op.key);
                    abort();
                }
                break;
            }
            case OP_READ: {
                hash_value_t read_result;
                auto error_code = qd_hash_get(qd, op.key, &read_result);
                // IMPORTANT!! verify the correctness
                if (error_code || read_result != op.value){
                    if(error_code)
                        printf("Key %x not found! @step%lu\n", op.key, hm.step_count);
                    else
                        std::cout << "READ value wrong !! " << "Read Key : [" << op.key << "] You Read: [" << read_result << "] Expected Value is: [" << op.value << "]" << std::endl; 
                    std::cout << "You should fix this BUG!" << std::endl;
                    return -1;
                }
                break;
            }
            case OP_UPDATE: {
                qd_hash_update(qd, op.key, op.value);
                break;
            }
            case OP_DELETE: {
                qd_hash_del(qd, op.key);
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

    qd_hash_destroy(qd);
    return 0;
}
