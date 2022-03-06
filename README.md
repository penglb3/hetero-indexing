# hetero-indexing
Indexing system for heterogeneous storages

## TODO (for dissertation. LOL idk if i can finish these.)
- [x] APIs (CRUD)
- [x] Implement backbone data structures: Hash table, ART.
- [x] Basic DRAM version
- [x] Fix value storage for ART.
- [x] Count-Min speedup: 
  - [x] One hash computation for multiple lines - chop 128 bit hash value into several.
  - [x] Sharing hash computation for hash table and Count-Min.
- [x] Implement index compact: hash refill + buffer lifting. 
- [ ] Smart data placing schedule. (Suppose `H` is the hash table, `T` the ART and `x` the key, `INB` Internal Node Buffer)
  - [x] `Insert(H, x)`: As is, because kicking keys in hash table for `x` can result in unnecessarily heavy write overhead.
  - [x] `Update(H, x)`: If the LRU key is less frequently used than updating key, then kick LRU key.
  - [ ] `Query(H, x)`: Mark/enqueue frequent data (define "frequent"?)
  - [x] `Delete(H, x)`: Set a trigger for compact (Say, `load_factor < MIN_LOAD_FACTOR and there are quite some in ART-INB`)
  - [x] `Expand(H)`: Compact.
  - [x] `Insert(T, x)`: We have the buffer.
  - [ ] `Update(T, x)`: As we go down the tree, if `freq(x) > freq(k)` for `k` in a buffer, replace `k` with `x` and kick `k` down
  - [ ] `Query(T, x)`: As we go down the tree, we want to keep an eye on low freq entry / empty space. 
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
