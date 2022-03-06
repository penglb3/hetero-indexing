# hetero-indexing
Indexing system for heterogeneous storages. 

## Requirements
- GCC supporting C11. You need to replace the atomic functions with other implementation if your compiler doesn't support C11.

## Test Environments
### Env #1
- CPU: AMD Ryzen 5 3600
- OS: WSL2-Ubuntu 18.04.5 LTS
- Compiler: gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04)
- Make: GNU Make 4.1 Built for x86_64-pc-linux-gnu

## TODO (for dissertation. LOL idk if i can finish these.)
- [x] APIs (CRUD)
- [x] Implement backbone data structures: Hash table, ART.
- [x] Basic DRAM version
- [x] Fix value storage for ART.
- [x] Count-Min speedup: 
  - [x] One hash computation for multiple lines - chop 128 bit hash value into several.
  - [x] Sharing hash computation for hash table and Count-Min.
- [x] Implement index compact: hash refill + buffer lifting. 
- [ ] Smart data placing schedule. (Suppose `H` is the hash table, `T` the ART and `x` the key, `INB` Internal Node Buffer. 
      **Basic Assumption / Design: Shallow layers of the ART should hold hot data, while deep layers should hold colder / io-type data**)
  - [x] `Insert(H, x)`: As is, because kicking keys in hash table for newcomer `x` can result in (unnecessarily) heavy write overhead. Plus, there's no easy way to tell whether we should kick a key down or not.
  - [x] `Update(H, x)`: If the LRU key is less frequently used than updating key, then kick LRU key.
  - [x] `Query(H, x)`: As is, because there is little we can do to elements in hash table.
  - [x] `Delete(H, x)`: Set a trigger for compact (Say, `load_factor < MIN_LOAD_FACTOR and there are quite some in ART-INB`)
  - [x] `Expand(H)`: Compact.
  - [x] `Insert(T, x)`: We have the buffer for this.
  - [ ] `Update(T, x)`: As we go down the tree, if `freq(x) > freq(k)` for `k` in a buffer, replace `k` with `x` and kick `k` down
  - [ ] `Query(T, x)`: Just like `Update(T, x)`
  - [x] `Delete(T, x)`: As is.
- [ ] Concurrency on DRAM:
  - [x] Hash
  - [ ] ART:
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
