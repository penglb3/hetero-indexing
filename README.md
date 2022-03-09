# hetero-indexing
Indexing system for heterogeneous storages. 

## Requirements
- GCC >= 4.9.0 (You need to replace the atomic functions with other implementation if your compiler doesn't support C11.)

## Test Environments
### Env #1
- CPU: AMD Ryzen 5 3600
- DRAM: 16G DDR4 3200
- OS: WSL2-Ubuntu 18.04.5 LTS
- Compiler: gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04)
- Make: GNU Make 4.1 Built for x86_64-pc-linux-gnu

## More than Hash + ART
(Suppose `H` is the hash table, `T` the ART and `x` the key, `INB` Internal Node Buffer. )

- ART internal node buffer(INB):
  - Caches a few KV pair on the path, reducing DRAM accesses and hence overhead. The design goal is that **shallow layers of the ART should hold hot data, while deep layers should hold colder / io-type data.**
  - Node split will create more buffer.
  - Internal node split 
  - When node type changes, the buffer will also be copied.
  - When a child is removed from a node4, buffered entries will become new children of that node4. So when a path compression takes place (a node4 only has 1 child), the buffer is guaranteed to be empty.
- `Update(H, x)`: SINK: If the LRU key is less frequently used than updating key, then kick LRU key.
- `Delete(H, x)`: A trigger is set for batch COMPACT (Say, `load_factor < MIN_LOAD_FACTOR and there are quite some in ART-INB`)
- `Expand(H)`: Auto COMPACT.
- `Update(T, x)`: FLOAT: If `freq(x) > freq(x')` for an `x'` in parent's buffer, replace `x'` with `x` and kick `x'` down.
- `Query(T, x)`: FLOAT: Just like `Update(T, x)`

## TODO (for dissertation. LOL idk if i can finish these.)
- [x] APIs (CRUD)
- [x] Implement backbone data structures: Hash table, ART.
- [x] Basic DRAM version
- [x] Fix value storage for ART.
- [x] Count-Min speedup: 
  - [x] One hash computation for multiple lines - chop 128 bit hash value into several.
  - [x] Sharing hash computation for hash table and Count-Min.
- [x] Implement index compact: hash refill + buffer lifting. 
- [x] Smart data placing schedule. 
- [ ] Concurrency on DRAM:
  - [x] Hash: TODO: Fix search to use 128-bit atomic load and return uint64_t.
  - [ ] ART: will use ROWEX
    - [ ] `add_child`
    - [ ] `split`
    - [ ] `remove_child`
    - [ ] `compaction`
## Future Works
- [ ] Test methods and perhaps a benchmark
- [ ] PMem version
- [ ] PMem Crash consistency
  - [ ] Hash: (Mechanism: CAS to mark key as occupied (invalid) -> store value (invalid) -> store real key (valid))
  - [ ] ART:
    - [ ] `add_child`
    - [ ] `split`
    - [ ] `remove_child`
    - [ ] `compaction`
- [ ] IO device version: storage and optimization
- [ ] Intelligent storage
