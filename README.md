# hetero-indexing
Indexing system for heterogeneous storages

## TODO (for dissertation)
- [x] APIs (CRUD)
- [x] Implement backbone data structures.
- [x] Very basic runnable DRAM version
- [ ] Use Count-Min Sketch[x] to distinguish hot data.
  - [x]Insertion: As is, because kicking keys in hash table for a new one can result in jittering.
  - [ ]Query: 
  - [x]Update: If the LRU key is less frequently used than updating key, then kick LRU key.
  - [ ]Delete: 
- [ ] Sharing hash computation for hash table and Count-Min.
- [ ] Fix value storage for ART.
## Future Works
- [ ] Concurrency on DRAM: Hash[x], ART[ ]
- [ ] Test methods and perhaps a benchmark
- [ ] PMem version
- [ ] PMem: Crash consistency
- [ ] IO device version: storage and optimization
- [ ] Intelligent storage
