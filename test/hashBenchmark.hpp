#ifndef _HASH_BENCHMARK_HPP_
#define _HASH_BENCHMARK_HPP_
#include <string>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <cassert>
#include <gflags/gflags.h>
#include <cstring>
#include <algorithm>
#include <map>
#ifdef GFLAGS_NAMESPACE
using namespace GFLAGS_NAMESPACE;
#else
using namespace gflags;
#endif

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <array>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


uint64_t get_media_writes_size(int n){
    #ifdef TEST_PMEM
    auto MediaWrites_out =  exec("ipmctl show -dimm -performance MediaWrites");
    std::cout << MediaWrites_out << std::endl;
    // std::cout << MediaWrites_out.substr(24+12, 34) << std::endl;
    // std::cout << MediaWrites_out.substr(24+12+34+24+12, 34) << std::endl;
    uint64_t MediaWrites_size = 0;
    for (int i = 0; i < n; i++){
        std::cout << MediaWrites_out.substr((24+12)*(i+1)+34*i, 34) << std::endl;
        MediaWrites_size += strtol(MediaWrites_out.substr((24+12)*(i+1)+34*i, 34).c_str(), NULL, 16);
    }
    std::cout << MediaWrites_size << std::endl;
    return MediaWrites_size;
    #else
    return 0;
    #endif
}

uint64_t get_media_reads_size(int n){
    #ifdef TEST_PMEM
    auto MediaReads_out =  exec("ipmctl show -dimm -performance MediaReads");
    std::cout << MediaReads_out << std::endl;
    uint64_t MediaReads_size = 0;
    for (int i = 0; i < n; i++){
        std::cout << MediaReads_out.substr((24+11)*(i+1)+34*i, 34) << std::endl;
        MediaReads_size += strtol(MediaReads_out.substr((24+11)*(i+1)+34*i, 34).c_str(), NULL, 16);
    }
    std::cout << MediaReads_size << std::endl;
    return MediaReads_size;
    #else
    return 0;
    #endif
}


DEFINE_string(mode, "tes", "workload type(default, ycsb)");

DEFINE_string(load_file, "NULL" ,"load workload file name");
DEFINE_string(run_file, "NULL" ,"run workload file name");
DEFINE_uint64(ops, 1000 ,"num of ops");
DEFINE_bool(check, true ,"check the correctness of result");

DEFINE_bool(with_update, false ,"add update op in default mode");
DEFINE_bool(with_delete, false ,"add delete op in default mode");


DEFINE_string(bench, "perf" ,"bench type");

typedef uint64_t hash_key_t;
typedef uint64_t hash_value_t;

#define NUM_OPS 6 // Number of all legal operations
#define YCSB_OP_LEN 6 // Length of an normal YCSB operation string, used to split hetero-indexing's memtype label
typedef enum OP_TYPE {
    OP_UNKNOWN,
    OP_INSERT,
    OP_READ,
    OP_SCAN,
    OP_UPDATE,
    OP_DELETE
}op_type_t;

op_type_t get_op_type_from_string(const std::string & s){
    if (s == "INSERT"){
        return OP_INSERT;
    } else if (s == "READ"){
        return OP_READ;
    } else if (s == "SCAN"){
        return OP_SCAN;
    } else if (s == "UPDATE"){
        return OP_UPDATE;
    } else if (s == "DELETE"){
        return OP_DELETE;
    } 
    return OP_UNKNOWN;
}

struct operation_t{
    op_type_t op;
    int info;
    hash_key_t key;
    hash_value_t value; // if op == OP_READ, value is the groundtrue result
    operation_t(op_type_t o, hash_key_t k, hash_value_t v, int i = 0)
        : op(o), key(k), value(v), info(i)
        {};
};

typedef struct Workload{
    std::string mode;
    std::string load_workload_file_name;
    std::string run_workload_file_name;
}workload_t;

workload_t this_workload;
std::string bench_type;

class HashBenchmark{
    std::string hash_name;
    std::string platform;
    bool running, bench_latency;

    // workload info
    std::string mode;
    std::string load_workload_file_name;
    std::string run_workload_file_name;

    // temp info
    std::string step_name;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point step_start, step_end;
    size_t start_mem_size;
    size_t end_mem_size;
    std::chrono::nanoseconds duration, acc_duration;
    size_t mem_size_diff;
    uint64_t MediaReads;
    uint64_t MediaWrites;
    int num_of_pm;
    std::vector<std::chrono::nanoseconds> latency;
public:
    int step_count;
    // workload data
    std::vector<double> ps;
    std::unordered_map<hash_key_t, hash_value_t> groundtrue_hashmap;
    std::vector<operation_t> load_ops;
    std::vector<operation_t>  run_ops;
    // some flags
    std::string bench_type;
    
    HashBenchmark(int argc, char ** argv, const std::string & hash_name, const std::string & platform){
        ParseCommandLineFlags(&argc, &argv, false);
        this->hash_name = hash_name;
        this->platform = platform;
        std::cout << "Init ..." << std::endl;
        std::cout << "mode : " << FLAGS_mode <<std::endl;
        std::cout << "bench : " << FLAGS_bench <<std::endl;
        std::cout << "check : " << FLAGS_check <<std::endl;
        std::cout << "with_update : " << FLAGS_with_update << std::endl;
        std::cout << "with_delete : " << FLAGS_with_delete << std::endl;
        std::cout << "load file name : " << FLAGS_load_file << std::endl;
        std::cout << "run file name : " << FLAGS_run_file << std::endl;
        num_of_pm = 2;
        std::vector<double> ps = {0.001, 0.005, 0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.95, 0.99, 0.999, 0.9999, 0.99999, 0.999999};
        this->ps = ps;
        // deal with args
        bench_type = FLAGS_bench;
        bench_latency = (bench_type == "latency");
        mode = FLAGS_mode;

        std::vector<int> load_op_counts(NUM_OPS, 0);
        std::vector<int> run_op_counts(NUM_OPS, 0);
        if (mode == "default"){
            uint64_t start_key = 1234567890123456789UL;
            uint64_t num_of_ops = FLAGS_ops; 
            load_workload_file_name = "None";
            run_workload_file_name = "None";
            std::cout << "start_key is " << start_key << std::endl;
            std::cout << "num_of_ops is " << num_of_ops << std::endl;
            // just for test
            for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                load_ops.push_back(operation_t{OP_INSERT, i, i+1});
                run_ops.push_back(operation_t{OP_READ, i, i+1});
            }
            if (FLAGS_with_update){
                for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                    run_ops.push_back(operation_t{OP_UPDATE, i, ~i});
                }
                for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                    run_ops.push_back(operation_t{OP_READ, i, ~i});
                }
            }
            if (FLAGS_with_delete){
                for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                    run_ops.push_back(operation_t{OP_DELETE, i, ~i});
                }
                for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                    run_ops.push_back(operation_t{OP_INSERT, i, ~i-1});
                }
                for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                    run_ops.push_back(operation_t{OP_READ, i, ~i-1});
                }
            }
        } else if (mode == "test-update-recover"){
            uint64_t start_key = 1234567890123456789UL;
            uint64_t num_of_ops = FLAGS_ops; 
            load_workload_file_name = "None";
            run_workload_file_name = "None";
            std::cout << "start_key is " << start_key << std::endl;
            std::cout << "num_of_ops is " << num_of_ops << std::endl;
            for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                load_ops.push_back(operation_t{OP_INSERT, i, i+1});
            }
            for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                load_ops.push_back(operation_t{OP_UPDATE, i, ~i});
                run_ops.push_back(operation_t{OP_READ, i, ~i});
            }
        } else if (mode == "test-delete-recover"){
            uint64_t start_key = 1234567890123456789UL;
            uint64_t num_of_ops = FLAGS_ops; 
            load_workload_file_name = "None";
            run_workload_file_name = "None";
            std::cout << "start_key is " << start_key << std::endl;
            std::cout << "num_of_ops is " << num_of_ops << std::endl;
            for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                load_ops.push_back(operation_t{OP_INSERT, i, i+1});
            }
            for (hash_key_t i = start_key; i < start_key+num_of_ops; i++){
                load_ops.push_back(operation_t{OP_DELETE, i, i});
                run_ops.push_back(operation_t{OP_READ, i, ~i});
            }
        } else if (mode == "ycsb") {
            std::string op_string; hash_key_t hash_key;
            std::cout << "reading workload files ..." << std::endl;
            // init workload from files

            // load
            std::string load_filename = FLAGS_load_file;
            std::cout << "load_filename : " << load_filename << std::endl;
            load_workload_file_name = load_filename;
            // readfile
            std::ifstream load_file_stream(load_filename);
            int info;
            while (load_file_stream >> op_string >> hash_key){
                info = 0;
                if(op_string.length() > YCSB_OP_LEN){
                    info = 1;
                    // op_string.resize(6);
                    op_string.erase(op_string.begin()+YCSB_OP_LEN, op_string.end());
                }
                op_type_t op_type = get_op_type_from_string(op_string);
                // std::cout << "[DEBUG] OP : " << op_string << " " << hash_key << std::endl;
                if (op_type == OP_UNKNOWN){
                    std::cout << "Warning : Unknown OP : " << op_string << " " << hash_key << std::endl;
                    continue;
                }
                load_ops.push_back(operation_t{op_type, hash_key, hash_key, info});
            }

            // run
            std::string run_filename = FLAGS_run_file;
            std::cout << "run_filename : " << run_filename << std::endl;
            run_workload_file_name = run_filename;
            // readfile
            std::ifstream run_file_stream(run_filename);
            while (run_file_stream >> op_string >> hash_key){
                op_type_t op_type = get_op_type_from_string(op_string);
                // std::cout << "[DEBUG] OP : " << op_string << " " << hash_key << std::endl;
                if (op_type == OP_UNKNOWN){
                    std::cout << "Warning : Unknown OP : " << op_string << " " << hash_key << std::endl;
                    continue;
                }
                // FIXME: update operation should modify the value ?
                if (op_type == OP_UPDATE){
                    // maybe it can modify the value once
                    run_ops.push_back(operation_t{op_type, hash_key, ~hash_key});
                } else if (op_type == OP_SCAN){
                    // FIXME: NOT IMPLEMENTED
                    int num_of_read;
                    run_file_stream >> num_of_read; 
                    continue;
                } else {
                    run_ops.push_back(operation_t{op_type, hash_key, hash_key+1});
                }
            }
        }
        for (auto op : load_ops){
            load_op_counts[int(op.op)]++;
        }
        for (auto op : run_ops){
            run_op_counts[int(op.op)]++;
        }
        
        std::cout << "[load] [total ] : " << load_ops.size() << std::endl;
        std::cout << "[load] [INSERT] : " << load_op_counts[OP_INSERT] <<std::endl;
        std::cout << "[load] [READ  ] : " << load_op_counts[OP_READ] <<std::endl;
        std::cout << "[load] [SCAN  ] : " << load_op_counts[OP_SCAN] <<std::endl;
        std::cout << "[load] [UPDATE] : " << load_op_counts[OP_UPDATE] <<std::endl;
        std::cout << "[load] [DELETE] : " << load_op_counts[OP_DELETE] <<std::endl;
        std::cout << "[run ] [total ] : " << run_ops.size() << std::endl;
        std::cout << "[run ] [INSERT] : " << run_op_counts[OP_INSERT] <<std::endl;
        std::cout << "[run ] [READ  ] : " << run_op_counts[OP_READ] <<std::endl;
        std::cout << "[run ] [SCAN  ] : " << run_op_counts[OP_SCAN] <<std::endl;
        std::cout << "[run ] [UPDATE] : " << run_op_counts[OP_UPDATE] <<std::endl;
        std::cout << "[run ] [DELETE] : " << run_op_counts[OP_DELETE] <<std::endl;

        // init ok
        if (FLAGS_check){
            HashBenchmark::groundtrue_hash_map(load_ops, run_ops);
        }
        auto mem_size = get_stable_mem_size();
        std::cout << "Maybe totally free memory\n";
        std::cout << "Init load and run operations ok!" << std::endl <<std::endl;
    }
    void Start(const std::string & name){
        MediaReads = get_media_reads_size(num_of_pm);
        MediaWrites = get_media_writes_size(num_of_pm);
        
        std::cout << std::endl << "Start Step :" << name << std::endl;
        step_name = name;
        if(bench_latency){
            if(name == "load")
                latency = std::vector<std::chrono::nanoseconds>(load_ops.size());
            else
                latency = std::vector<std::chrono::nanoseconds>(run_ops.size());
        }
        step_count = 0;
        start_mem_size = HashBenchmark::get_mem_size();
        start_time = std::chrono::system_clock::now();
        step_start = start_time;
    }

    void End(){
        duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now() - start_time);
        end_mem_size = get_stable_mem_size();
        mem_size_diff = end_mem_size - start_mem_size;

        MediaReads = (get_media_reads_size(num_of_pm) - MediaReads)/16;
        MediaWrites = (get_media_writes_size(num_of_pm) - MediaWrites)/16; // 16 = 1064/64
    }

    void Step(){
        if(!bench_latency) return;
        step_end = std::chrono::system_clock::now();
        latency[step_count++] = std::chrono::duration_cast<std::chrono::nanoseconds>(step_end - step_start);
        step_start = step_end;
    }

    void Print(const std::map<std::string, double> & parameters){
        print_benchmarks_result(parameters);
        step_name = "None";
    }

    inline bool shouldCheck(){
        return FLAGS_check;
    }


    void print_benchmarks_result(const std::map<std::string, double> & parameters){
        uint64_t throughput;
        uint64_t num_of_ops;
        std::string workload_file_name;
        if (step_name == "load"){
            throughput = 1000*1000*1000 * load_ops.size()/duration.count();
            num_of_ops = load_ops.size();
            workload_file_name = load_workload_file_name;
        } else if (step_name == "run"){
            throughput = 1000*1000*1000 * run_ops.size()/duration.count();
            num_of_ops = run_ops.size();
            workload_file_name = run_workload_file_name;
        }
        std::chrono::nanoseconds z=std::chrono::nanoseconds::zero(),
            median=z, P5=z, P25=z, P75=z, P95=z;
        if(bench_latency){
            std::sort(latency.begin(), latency.end());
            median = latency[latency.size() / 2];
            if (latency.size() & 1)
                median = (median + latency[latency.size()/2+1]) / 2;
            auto per = latency.size() / 100;
            P5 = latency[per * 5],
            P25 = latency[per * 25],
            P75 = latency[per * 75],
            P95 = latency[per * 95];
        }

        std::cout 
                << std::fixed << std::left << std::setw(16) << ANSI_COLOR_RED "[results_info] " ANSI_COLOR_RESET
                << "platform"
                << "," << "mode"
                << "," << "step_name" 
                << "," << ANSI_COLOR_MAGENTA "num_of_ops" ANSI_COLOR_RESET 
                << "," << ANSI_COLOR_BLUE "duration(ms)" ANSI_COLOR_RESET
                << "," << "hash_name"
                << "," << ANSI_COLOR_YELLOW "throughput(op/s)" ANSI_COLOR_RESET
                << "," << ANSI_COLOR_GREEN "avg_latency(ns)" ANSI_COLOR_RESET
                << "," << ANSI_COLOR_CYAN "mem_diff_size(KB)" ANSI_COLOR_RESET
                << "," << "start_mem_size(KB)"
                << "," << "end_mem_size(KB)"
                << "," << ANSI_COLOR_RED "median_latency(ns)" ANSI_COLOR_RESET
                << "," << "P[5-25-75-95]"
                << "," << "workload_file_name"
                #ifdef TEST_PMEM
                << "," << "MediaReads(KB)" 
                << "," << "MediaWrites(KB)"
                #endif
                ;
                
        for (auto k : parameters){
            std::cout << "," << k.first;
        }
        std::cout << std::endl;

        std::cout
                << std::fixed << std::left << std::setw(16) << ANSI_COLOR_RED "[results] " ANSI_COLOR_RESET
                << platform
                << "," << mode
                << "," << step_name
                << ANSI_COLOR_RESET "," ANSI_COLOR_MAGENTA << num_of_ops 
                << ANSI_COLOR_RESET "," ANSI_COLOR_BLUE << duration.count()/(1000*1000)
                << ANSI_COLOR_RESET "," << hash_name
                << ANSI_COLOR_RESET "," ANSI_COLOR_YELLOW << std::setprecision(1) << throughput 
                << ANSI_COLOR_RESET "," ANSI_COLOR_GREEN << std::setprecision(3) << 1000.0*1000.0*1000.0/throughput
                << ANSI_COLOR_RESET "," ANSI_COLOR_CYAN << mem_size_diff
                << ANSI_COLOR_RESET "," << start_mem_size
                << "," << end_mem_size
                << "," ANSI_COLOR_RED << std::setprecision(3)  << median.count()
                << ANSI_COLOR_RESET ",[" << std::setprecision(3) << P5.count() << '-' 
                                         << std::setprecision(3) << P25.count() << '-' 
                                         << std::setprecision(3) << P75.count() << '-'
                                         << std::setprecision(3) << P95.count() << ']'
                << "," << workload_file_name
                #ifdef TEST_PMEM
                << "," << MediaReads
                << "," << MediaWrites
                #endif
                << ANSI_COLOR_RESET;
        for (auto k : parameters){
            std::cout << "," << std::setprecision(4) << k.second;
        }
        std::cout << std::endl;
    }

    // https://zhiqiang.org/coding/get-pysical-memory-used-by-process-in-cpp.html
    static size_t get_mem_size(){
        FILE* file = fopen("/proc/self/status", "r");
        size_t result = -1;
        char line[128];

        while (fgets(line, 128, file) != nullptr) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                int len = strlen(line);

                const char* p = line;
                for (; std::isdigit(*p) == false; ++p) {}

                line[len - 3] = 0;
                result = atoll(p);

                break;
            }
        }
        fclose(file);
        return result;
    }
    static size_t get_stable_mem_size(){
        auto mem_size = get_mem_size();
        std::cout << "mem_size is :" << mem_size << std::endl;
        std::cout << "wait for mem_size to be stable" << std::endl;
        sleep(2);
        while (abs(get_mem_size()-mem_size) > 10000){
            mem_size = get_mem_size();
            std::cout << "mem_size is :" << mem_size << std::endl;
            std::cout << "wait for mem_size to be stable" << std::endl;
            std::cout << "sleep .." << std::endl;
            sleep(2);
        }
        return mem_size;
    }

    static std::unordered_map<hash_key_t, hash_value_t> groundtrue_hash_map(std::vector<operation_t> & load_ops, std::vector<operation_t> & run_ops){
        std::cout << "Generate results of  groundtrue hash map(std::unordered_map) ... " << std::endl;
        // update op.value in OP_READ
        std::unordered_map<hash_key_t, hash_value_t> test_map;
        // load
        for (auto op : load_ops ){
            switch (op.op)
            {
                case OP_INSERT: {
                    test_map.insert({op.key, op.value});
                    break;
                }
                case OP_READ: {
                    auto read_result = test_map[op.key];
                    // IMPORTANT!!
                    op.value = read_result;
                    break;
                }
                case OP_UPDATE: {
                    test_map[op.key] = op.value;
                    break;
                }
                case OP_DELETE: {
                    test_map.erase(op.key);
                    break;
                }
                default: {
                    std::cout << "Unknown OP In Load : " << op.op << std::endl;
                    break;
                }
            }
        }

        // run
        for (auto & op : run_ops ){
            switch (op.op)
            {
                case OP_INSERT: {
                    test_map.insert({op.key, op.value});
                    break;
                }
                case OP_READ: {
                    auto read_result = test_map[op.key];
                    // IMPORTANT!!
                    op.value = read_result;
                    break;
                }
                case OP_UPDATE: {
                    test_map[op.key] = op.value;
                    break;
                }
                case OP_DELETE: {
                    test_map.erase(op.key);
                    break;
                }
                default: {
                    std::cout << "Unknown OP In Run : " << op.op << std::endl;
                    break;
                }
            }
        }
        return std::move(test_map);
    }


};

#endif /* _HASH_BENCHMARK_HPP_ */
