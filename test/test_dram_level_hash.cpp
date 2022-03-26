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

// An example to test the performance of map
void test_perf(HashBenchmark & hm){
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
                    return ;
                }
                reinsert = 0;
                break;
            }
            default: {
                std::cout << "Unknown OP In Load : " << op.op << std::endl;
                break;
            }
        }
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
                    return ;
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
                    return ;
                }
                if (*result_ptr != op.value){
                    std::cout << "READ value wrong !! " << "Read Key : [" << op.key << "] You Read: [" << *result_ptr << "] Expected Value is: [" << op.value << "]" << std::endl; 
                    std::cout << "You should correct this BUG!" << std::endl;
                    return ;
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
    }

    hm.End();
    hm.Print(parameters);
    return ;
}

// void test_latency(HashBenchmark & hm){
//     std::map<std::string, double> parameters;
//     const std::vector<operation_t>& load_ops = hm.load_ops;
//     const std::vector<operation_t>& run_ops = hm.run_ops;

//     // Step 1: create (if not exist) and open the pool
//     std::string pool_file_name = "/mnt/pmem/wyf_test_pool";
//     std::remove(pool_file_name.c_str());
//     Allocator::Initialize(pool_file_name.c_str(),  1024ul * 1024ul * 1024ul * 10ul);

//     // Step 2: Allocate the initial space for the hash table on PM and get the
//     // root; we use Dash-EH in this case.
//     // Hash<uint64_t> *hash_table = reinterpret_cast<Hash<uint64_t> *>(
//         // Allocator::GetRoot(sizeof(extendible::Finger_EH<uint64_t>)));
//     Hash<uint64_t> *hash_table = reinterpret_cast<Hash<uint64_t> *>(
//         Allocator::GetRoot(sizeof(level::LevelHashing<uint64_t>)));
        
        

//     // Step 3: Initialize the hash table
//     // During initialization phase, allocate 64 segments for Dash-EH
//     size_t segment_number = 64;
//     // new (hash_table) cceh::CCEH<uint64_t><uint64_t>(
//     //         segment_number, Allocator::Get()->pm_pool_);
//     new (hash_table) level::LevelHashing<uint64_t>();
//     int level_size = 13;
//     level::initialize_level(Allocator::Get()->pm_pool_,
//                             reinterpret_cast<level::LevelHashing<uint64_t> *>(hash_table),
//                             &level_size);


//     auto cur_time = std::chrono::system_clock::now();
//     auto pre_time = std::chrono::system_clock::now();
//     std::vector<uint64_t> load_latencys(load_ops.size(),0);
//     uint64_t load_ops_size = load_ops.size();
//     hm.Start("load");

//     // load
//     // auto epoch_guard = Allocator::AquireEpochGuard();
//     for (int i = 0; i < load_ops_size; i++){
//         auto op = load_ops[i];
//         pre_time = std::chrono::system_clock::now();
//         switch (op.op)
//         {
//             case OP_INSERT: {
//                 auto ret = hash_table->Insert(op.key, op.value, true);
//                 break;
//             }
//             default: {
//                 std::cout << "Unknown OP In Load : " << op.op << std::endl;
//                 break;
//             }
//         }
//         cur_time = std::chrono::system_clock::now();
//         load_latencys[i] =  std::chrono::duration_cast<std::chrono::nanoseconds>(cur_time-pre_time).count();

//     }

//     hm.End();
//     parameters["0load_factor"] = hash_table->getLoadFactor();
//     hm.Print(parameters);


//     std::sort(load_latencys.begin(), load_latencys.end());
//     std::cout << "min : " << *(load_latencys.begin()) << std::endl;
//     for (int i = 1; i <= 10; i++){
//         std::cout << "max : " << *(load_latencys.end()-i) << std::endl;
//     }
//     for (auto p : hm.ps){
//         std::cout << "p "  << p << " : " << load_latencys[(uint64_t)(load_latencys.size()*p)] << std::endl;
//     }

//     // run 
//     std::vector<uint64_t> run_latencys(run_ops.size(),0);
//     uint64_t run_ops_size = run_ops.size();


//     // run 
//     hm.Start("run");

//     for (int i = 0; i < run_ops_size; i++){
//         auto op = run_ops[i];
//         pre_time = std::chrono::system_clock::now();
//         switch (op.op)
//         {
//             case OP_INSERT: {
//                 auto ret = hash_table->Insert(op.key, op.value, true);
//                 break;
//             }
//             case OP_READ: {

//                 hash_value_t read_result = hash_table->Get(op.key, true);
//                 if (hm.shouldCheck()){
//                     if (read_result != op.value){
//                         std::cout << "READ value wrong !! " << "Read Key : [" << op.key << "] You Read: [" << read_result << "] Expected Value is: [" << op.value << "]" << std::endl; 
//                         std::cout << "You should correct this BUG!" << std::endl;
//                         return ;
//                     }
//                 }
//                 break;
//             }
//             case OP_UPDATE: {
//                 std::cout << "Not Implemented OP[UPDATE] In Run : " << op.op << std::endl;
//                 break;
//             }
//             case OP_DELETE: {
//                 auto ret = hash_table->Delete(op.key, true);
//                 if (hm.shouldCheck()){
//                     hash_value_t read_result = hash_table->Get(op.key, true);
//                     if (read_result != NONE){
//                         std::cout << "READ an deleted key, it seems not to be deleted successfully." << std::endl; 
//                         std::cout << "You should correct this BUG!" << std::endl;
//                         return ;
//                     }
//                 }
//                 // std::cout << "Not Implemented OP[DELETE] In Run : " << op.op << std::endl;
//                 break;
//             }
//             default: {
//                 std::cout << "Unknown OP In Run : " << op.op << std::endl;
//                 break;
//             }
//         }
//         cur_time = std::chrono::system_clock::now();
//         run_latencys[i] =  std::chrono::duration_cast<std::chrono::nanoseconds>(cur_time-pre_time).count();

//     }

//     hm.End();
//     parameters["0load_factor"] = hash_table->getLoadFactor();
//     hm.Print(parameters);

//     std::sort(run_latencys.begin(), run_latencys.end());
//     std::cout << "min : " << *(run_latencys.begin()) << std::endl;
//     for (int i = 1; i <= 10; i++){
//         std::cout << "max : " << *(run_latencys.end()-i) << std::endl;
//     }
//     for (auto p : hm.ps){
//         std::cout << "p " <<  p << " : " << run_latencys[(uint64_t)(run_latencys.size()*p)] << std::endl;
//     }

//     std::remove(pool_file_name.c_str());
//     std::cout << "Remove pool file : " << pool_file_name << std::endl;

//     return ;
// }

int main(int argc, char ** argv){
    HashBenchmark hm(argc, argv, "Level-hashing", "DRAM");
    if (hm.bench_type == "perf"){
        test_perf(hm);
    // } else if (hm.bench_type == "latency"){
        // test_latency(hm);
    } else {
        std::cout << "ERROR !!" << std::endl;
    }
    return 0;
}
