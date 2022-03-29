#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdalign.h>
__BEGIN_DECLS
#define IO_TYPE 0
#define MEM_TYPE 1
#define SSD_TYPE 2
#define HDD_TYPE 3
//* KEY and VALUES are of of char string type.
#define KEY_LEN 8
#define VAL_LEN 8

typedef struct entry{
    alignas(16) uint8_t key[KEY_LEN];
    uint8_t value[VAL_LEN];
} entry;

#define BIN_CAPACITY 8

// ------------- SPECIAL KEY VALUES -----------------
#define EMPTY_FLAG 0
#define OCCUPIED_FLAG 1
#define IS_SPECIAL_KEY(key) ((*(uint64_t*)key & ~1) == 0)
#define IS_SPECIAL_KEY_U64(key) ((key & ~1) == 0)
#define IS_OCCUPIED(key) ((*(uint64_t*)(key) == 1)
#define IS_EMPTY(key) ((*(uint64_t*)(key) == 0)

// ------------- FUNCTIONALITY SWITCHES -------------
// #define USE_CM 1
#if defined(USE_CM) && !defined(RAW_ART) // Following functionality requires Count-MIN
// Note that you can disable ART buffer by undef-ing BUF_LEN, which should be somewhere above ;) 
#define SDR_SINK 1 // To disable hash update SINK, just undef this. 
#define SDR_FLOAT 1 // Float mode: 0 = disabled, 1 = INB only, 2 = leaf nodes only, 3 = all
#endif

#define COMPACT 1 // To disable index COMPACT, just undef this.

// #define COUNTMIN_CD_FACTOR 0.99 // To disable Count-Min cooldown, disable either of these
// #define COUNTMIN_CD_INTERVAL 200

// ------------------- PARAMETERS -------------------
#define EST_SCALE 1.05 // We want to be more certain when comparing ESTIMATED frequency and prevent jittering.
#define EST_DIFF 10 // Prevent jittering by adding some constant (say 10), for now we use 0 for test. 
#define COMPACT_START_LOAD_FACTOR 0.6 // When hash's load factor is lower than this value after a delete, compact begins.
#define MAX_COMPACT_LOAD_FACTOR 0.875 // When hash's load factor reach this value during compacting, compact ends.
#define MAX_TREE_HASH_RATIO 4
// ------------------- HASH ---------------------


typedef struct entry_bin{
    entry data[BIN_CAPACITY];
} bin;

typedef struct hash_sys{
    uint32_t seed;
    uint64_t size;
    uint64_t count;
    bin* entries;
} hash_sys;

// ------------------- ART ---------------------
#define NODE4   1
#define NODE16  2
#define NODE48  3
#define NODE256 4
#ifndef RAW_ART
#define BUF_LEN 3 // To disable ART INB, just undef this.
#endif
#define MAX_PREFIX_LEN KEY_LEN

/**
 * This struct is included as part
 * of all the various node sizes
 */

typedef struct art_node{
    uint8_t partial_len;
    uint8_t type;
    uint8_t num_children;
    unsigned char partial[MAX_PREFIX_LEN];
#ifdef BUF_LEN
    entry buffer[BUF_LEN];
#endif
} art_node;

/**
 * Main struct, points to root.
 */
typedef struct art_tree{
    art_node *root;
    uint64_t size;
    uint64_t buffer_count;
} art_tree;
// ------------------ DATA SKETCH -----------------
typedef struct data_sketch {
    uint32_t width, depth;
    uint32_t seed;
    uint32_t shift;
    #if defined(COUNTMIN_CD_FACTOR) && defined(COUNTMIN_CD_INTERVAL)
    uint32_t cd_countdown;
    #endif
    uint32_t *counts;
} sketch;

// ------------------- HETERO INDEX ---------------------

typedef struct index_sys{
    uint8_t has_special_key[2], special_key_val[2][VAL_LEN];
    sketch *cm, *memb;
    hash_sys* hash;
    art_tree* tree;
} index_sys;

__END_DECLS
#endif // COMMON_H