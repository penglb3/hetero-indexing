#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define MEM_TYPE 1
#define SSD_TYPE 2
#define HDD_TYPE 3
//* KEY and VALUES are of of char string type.
#define KEY_LEN 8
#define VAL_LEN 8

#define UPDATE_KICK 1 // To disable hash update kicking, just undef this. 
#define BUF_LEN 3 // To disable ART INB, just undef this.
#define SMART_REPLACING 1 // To disable ART update smart replacing, just undef this
#define COMPACT 1
typedef struct entry{
    uint8_t _Alignas(16) key[KEY_LEN];
    uint8_t value[VAL_LEN];
} entry;
// ------------------- HASH.H ---------------------
#define BIN_CAPACITY 8

typedef struct entry_bin{
    entry data[BIN_CAPACITY];
} bin;

typedef struct hash_sys{
    uint32_t seed;
    uint64_t size;
    uint64_t count;
    bin* entries;
} hash_sys;

// ------------------- ART.H ---------------------
#define NODE4   1
#define NODE16  2
#define NODE48  3
#define NODE256 4

#define MAX_PREFIX_LEN KEY_LEN

/**
 * This struct is included as part
 * of all the various node sizes
 */

typedef struct art_node{
    uint32_t partial_len;
    uint8_t type;
    uint8_t num_children;
    unsigned char partial[MAX_PREFIX_LEN];
#ifdef BUF_LEN
    entry buffer[BUF_LEN];
#endif
} art_node;

/**
 * Small node with only 4 children
 */
typedef struct art_node4{
    art_node n;
    unsigned char keys[4];
    art_node *children[4];
} art_node4;

/**
 * Node with 16 children
 */
typedef struct art_node16{
    art_node n;
    unsigned char keys[16];
    art_node *children[16];
} art_node16;

/**
 * Node with 48 children, but
 * a full 256 byte field.
 */
typedef struct art_node48{
    art_node n;
    unsigned char keys[256];
    art_node *children[48];
} art_node48;

/**
 * Full node with 256 children
 */
typedef struct art_node256{
    art_node n;
    art_node *children[256];
} art_node256;

/**
 * Represents a leaf. These are
 * of arbitrary size, as they include the key.
 */
typedef struct art_leaf{
    uint8_t value[VAL_LEN];
    uint32_t key_len;
    unsigned char key[];
} art_leaf;

/**
 * Main struct, points to root.
 */
typedef struct art_tree{
    art_node *root;
    uint64_t size;
    uint64_t buffer_count;
} art_tree;

// ------------------- HETERO.H ---------------------
#include "data_sketch.h"
#define EMPTY_FLAG 0
#define OCCUPIED_FLAG 1
#define IS_SPECIAL_KEY(key) ((*(uint64_t*)key & ~1) == 0)
#define IS_SPECIAL_KEY_U64(key) ((key & ~1) == 0)
typedef struct index_sys{
    uint8_t has_special_key[2], special_key_val[2][VAL_LEN];
    sketch *cm, *memb;
    hash_sys* hash;
    art_tree* tree;
} index_sys;

int debug_count;

// We want to be more certain 
#define EST_SCALE 1.02 
#define EST_DIFF 0 // For now we set to 0 for test.
#endif // COMMON_H