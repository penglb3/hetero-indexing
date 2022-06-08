# hetero-indexing
Indexing system for heterogeneous storages. 

## Requirements
- GFLAGS
- GCC >= 4.9.0 (You need to replace the atomic functions with other implementation if your compiler doesn't support C11.)

## Test Environments
### Env #1
- CPU: Intel Xeon Gold 6230N
- DRAM: 192GB DDR4 2666
- OS: CentOS 7 Linux
- Compiler: GCC 10.2.1 (Red Hat Developer Toolset 10.1)

### Env #2 (validation only)
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
  - When a child is removed from a node4, buffered entries will become new children of that node4. So when a path compression takes place (i.e., when a node4 has only 1 child), the buffer is guaranteed to be empty.
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
- [x] Smart data placing schedule (SINK, FLOAT). 
- [x] Count-Min Auto cooldown.
- [ ] Optimize CMS usage to make query faster (than insert, presumably).
- [ ] Concurrency on DRAM:
  - [x] Hash
  - [ ] ART: will use ROWEX
    - [ ] `local modifications`:
      - [ ] ` atomic accesses`: node4 & node16's `key` bytes, node48's `child indexes` and all `children pointers`. 
      - [ ] For node4 and node16, key sorting need to be deleted. When insert, just add new keys at the end. When delete, just set children ptr to NULL
    - [ ] `node replacement`: [1] lock node and parent. [2] copy node info to new node. [3] change parent's child pointer to new node. [4] unlock node and mark as OBSOLETE.
    - [ ] `internal split`: [1] install new node [2] truncate the prefix (change both prefix and its length in single atomic store)
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

## Codes Borrowed / Adapted
- Murmur3 from https://github.com/PeterScott/murmur3
- ART from https://github.com/armon/libart
- EBR GC from https://github.com/rmind/libqsbr
