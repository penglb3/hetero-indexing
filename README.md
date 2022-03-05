# hetero-indexing
Indexing system for heterogeneous storages

## TODO (for dissertation)
- [x] APIs (CRUD)
- [x] Implement backbone data structures: Hash table, ART.
- [x] Basic DRAM version
- [x] Fix value storage for ART.
- [x] Count-Min speedup: 
  - [x] One hash computation for multiple lines - chop 128 bit hash value into several.
  - [x] Sharing hash computation for hash table and Count-Min.
- [ ] Use Count-Min Sketch [x] to distinguish hot data.
  - [x] Insertion: As is, because kicking keys in hash table for a new one can result in jittering.
  - [x] Update: If the LRU key is less frequently used than updating key, then kick LRU key.
  - [ ] Query: 
  - [ ] Delete: 
- [ ] Concurrency on DRAM: Hash [x], ART [ ]
## Future Works
- [ ] Test methods and perhaps a benchmark
- [ ] PMem version
- [ ] PMem: Crash consistency
- [ ] IO device version: storage and optimization
- [ ] Intelligent storage
