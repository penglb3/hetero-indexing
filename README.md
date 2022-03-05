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
- [ ] Use Count-Min Sketch [x] to distinguish hot data. (Suppose `H` is the hash table, `T` the ART and `x` the key)
  - [x] `Insert(H, x)`: As is, because kicking keys in hash table for `x` can result in unnecessarily heavy write overhead.
  - [x] `Update(H, x)`: If the LRU key is less frequently used than updating key, then kick LRU key.
  - [ ] `Query(H, x)`: Inc `freq(x)`, and mark/enqueue frequent data (define "frequent"?)
  - [ ] `Delete(H, x)`: Should we try to bring up some data from ART node buffer?
  - [ ] `Expand(H)`: Bring up data in ART node buffer into `H`
  - [x] `Insert(T, x)`: We have the buffer.
  - [ ] `Update(T, x)`: As we go down the tree, we want to keep an eye on low freq entry / empty space
  - [ ] `Query(T, x)`: Inc `freq(x)`, and mark/enqueue frequent data (define "frequent"?)
  - [ ] `Delete(T, x)`: Should we try to bring up some data from ART node buffer?
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
  - [ ] Hash: (Mechanism: CAS to mark as occupied -> store value -> store key (valid).)
  - [ ] ART:
    - [ ] `add_child`
    - [ ] `split`
    - [ ] `remove_child`
    - [ ] `compaction`
- [ ] IO device version: storage and optimization
- [ ] Intelligent storage
