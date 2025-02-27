#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "art.h"
#include "atomic.h"
#include "data_sketch.h"
#include "hash.h"
#ifdef __i386__
    #include <emmintrin.h>
#else
#ifdef __amd64__
    #include <emmintrin.h>
#endif
#endif

// Code adapted from https://github.com/armon/libart

/**
 * Macros to manipulate pointer tags
 */
#define IS_LEAF(x) (((uintptr_t)x & 1))
#define SET_LEAF(x) ((void*)((uintptr_t)x | 1))
#define LEAF_RAW(x) ((art_leaf*)((void*)((uintptr_t)x & ~1)))
#define INSERT_FOUND_KEY_FLAG 2
#define INSERT_IS_IN_BUFFER_FLAG 1
#define INSERT_FOUND_KEY(x) (x & INSERT_FOUND_KEY_FLAG)
#define INSERT_IS_IN_BUFFER(x) (x & INSERT_IS_IN_BUFFER_FLAG)

static void* recursive_insert(art_node *n, art_node **ref, const unsigned char *key, int key_len, void *value, int depth, int *old, int replace);
static int prefix_mismatch(const art_node *n, const unsigned char *key, int key_len, int depth);
static int remove_child(art_node *n, art_node **ref, unsigned char c, art_node **l, int depth);
/**
 * Allocates a node of the given type,
 * initializes to zero and sets the type.
 */
static art_node* alloc_node(uint8_t type) {
    art_node* n;
    switch (type) {
        case NODE4:
            n = (art_node*)calloc(1, sizeof(art_node4));
            break;
        case NODE16:
            n = (art_node*)calloc(1, sizeof(art_node16));
            break;
        case NODE48:
            n = (art_node*)calloc(1, sizeof(art_node48));
            break;
        case NODE256:
            n = (art_node*)calloc(1, sizeof(art_node256));
            break;
        default:
            abort();
    }
    n->type = type;
    return n;
}

int art_tree_init(art_tree *t) {
    t->root = NULL;
    t->size = 0;
    t->buffer_count = 0;
    return 0;
}

// Recursively destroys the tree
static void destroy_node(art_node *n) {
    // Break if null
    if (!n) return;

    // Special case leafs
    if (IS_LEAF(n)) {
        free(LEAF_RAW(n));
        return;
    }

    // Handle each node type
    int i, idx;
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    switch (n->type) {
        case NODE4:
            p.p1 = (art_node4*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p1->children[i]);
            }
            break;

        case NODE16:
            p.p2 = (art_node16*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p2->children[i]);
            }
            break;

        case NODE48:
            p.p3 = (art_node48*)n;
            for (i=0;i<256;i++) {
                idx = ((art_node48*)n)->keys[i]; 
                if (!idx) continue; 
                destroy_node(p.p3->children[idx-1]);
            }
            break;

        case NODE256:
            p.p4 = (art_node256*)n;
            for (i=0;i<256;i++) {
                if (p.p4->children[i])
                    destroy_node(p.p4->children[i]);
            }
            break;

        default:
            abort();
    }

    // Free ourself on the way up
    free(n);
}

int art_tree_destroy(art_tree *t) {
    destroy_node(t->root);
    return 0;
}

/**
 * Returns the size of the ART tree.
 */

#ifndef BROKEN_GCC_C99_INLINE
extern inline uint64_t art_size(art_tree *t);
#endif

static art_node** find_child(art_node *n, unsigned char c) {
    int i, mask, bitfield;
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    switch (n->type) {
        case NODE4:
            p.p1 = (art_node4*)n;
            for (i=0 ; i < n->num_children; i++) {
		/* this cast works around a bug in gcc 5.1 when unrolling loops
		 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59124
		 */
                if (((unsigned char*)p.p1->keys)[i] == c)
                    return &p.p1->children[i];
            }
            break;

        {
        case NODE16:
            p.p2 = (art_node16*)n;

            // support non-86 architectures
            #ifdef __i386__
                // Compare the key to all 16 stored keys
                __m128i cmp;
                cmp = _mm_cmpeq_epi8(_mm_set1_epi8(c),
                        _mm_loadu_si128((__m128i*)p.p2->keys));
                
                // Use a mask to ignore children that don't exist
                mask = (1 << n->num_children) - 1;
                bitfield = _mm_movemask_epi8(cmp) & mask;
            #else
            #ifdef __amd64__
                // Compare the key to all 16 stored keys
                __m128i cmp;
                cmp = _mm_cmpeq_epi8(_mm_set1_epi8(c),
                        _mm_loadu_si128((__m128i*)p.p2->keys));

                // Use a mask to ignore children that don't exist
                mask = (1 << n->num_children) - 1;
                bitfield = _mm_movemask_epi8(cmp) & mask;
            #else
                // Compare the key to all 16 stored keys
                bitfield = 0;
                for (i = 0; i < 16; ++i) {
                    if (p.p2->keys[i] == c)
                        bitfield |= (1 << i);
                }

                // Use a mask to ignore children that don't exist
                mask = (1 << n->num_children) - 1;
                bitfield &= mask;
            #endif
            #endif

            /*
             * If we have a match (any bit set) then we can
             * return the pointer match using ctz to get
             * the index.
             */
            if (bitfield)
                return &p.p2->children[__builtin_ctz(bitfield)];
            break;
        }

        case NODE48:
            p.p3 = (art_node48*)n;
            i = p.p3->keys[c];
            if (i)
                return &p.p3->children[i-1];
            break;

        case NODE256:
            p.p4 = (art_node256*)n;
            if (p.p4->children[c])
                return &p.p4->children[c];
            break;

        default:
            abort();
    }
    return NULL;
}

// Simple inlined if
static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

/**
 * Returns the number of prefix characters shared between
 * the key and node.
 */
static int check_prefix(const art_node *n, const unsigned char *key, int key_len, int depth) {
    int max_cmp = min(min(n->partial_len, MAX_PREFIX_LEN), key_len - depth);
    int idx;
    for (idx=0; idx < max_cmp; idx++) {
        if (n->partial[idx] != key[depth+idx])
            return idx;
    }
    return idx;
}

/**
 * Checks if a leaf matches
 * @return 0 on success.
 */
static int leaf_matches(const art_leaf *n, const unsigned char *key, int key_len, int depth) {
    (void)depth;
    // Fail if the key lengths are different
    if (KEY_LEN != (uint32_t)key_len) return 1;

    // Compare the keys starting at the depth
    return memcmp(n->key, key, key_len);
}

static inline double max(double a, double b){
    return a > b ? a : b;
}

#define FLOAT_RESULT_ROOT 1 // Key is at root node, and we inserted it into hash table.
#define FLOAT_RESULT_MOVE 2 // We found an empty slot in parent and moved KV over there
#define FLOAT_RESULT_REINSERT 3 // We found an LRU key in parent, replaced it with key and then reinserted it.
#define FLOAT_RESULT_SWAP 4 // We found an LRU key swappable with key, which we did.
#define PACK_INFO(freq, i, depth, d_parent) ((((uint64_t)(((d_parent<<8)|depth)<<8)|i)<<32) | freq)
#define VALUE_TO_STORE(uv_ptr, ev_ptr) (uv_ptr ? (*(uint64_t*)uv_ptr) : (atomic_load(ev_ptr)))
/**
 * @brief Try to FLOAT key in node n to its parent node.
 * 
 * @param ind The index system
 * @param key The key FOUND in art node n.
 * @param info integer information package generated with PACK_INFO
 * @param n POINTER REFERENCE to the node containing key
 * @param parent POINTER REFERENCE to n's parent node. This is a must for proper memory management.
 * @param update_value The value to update. Pass NULL to use original value.
 * @return non-zero operation code if float is done successfully, 0 otherwise 
 */
#ifdef BUF_LEN
static int art_float(index_sys* ind, const uint8_t *key, const uint64_t info, art_node** ref_n, art_node** ref_parent, const void* update_value) {
    art_node* n = *ref_n, *parent;
    int f, delta_bcount = 0;
    const int freq = info & 0xffffffff, i = (info >> 32) & 0xff, depth = (info >> 40) & 0xff, depth_parent = (info >> 48) & 0xff;
    uint64_t key_lru = EMPTY_FLAG, val, key_i;
    entry* entry_lru = NULL;
    uint64_t *key_ptr = IS_LEAF(n) ? (uint64_t*)(LEAF_RAW(n)->key) : (uint64_t*)n->buffer[i].key,
             *value_ptr = IS_LEAF(n) ? (uint64_t*)(LEAF_RAW(n)->value) : (uint64_t*)n->buffer[i].value;
    if(!ref_parent){
        if(update_value || hash_insert_nocheck(ind->hash, key, (const uint8_t*)value_ptr))
            return 0;
        *key_ptr = EMPTY_FLAG;
        if(!IS_LEAF(n))
            delta_bcount = 1;
        else
            delta_bcount = remove_child(ind->tree->root, & ind->tree->root, key[depth_parent], ref_n, depth_parent);
        if(delta_bcount)
            ind->tree->buffer_count -= delta_bcount;
        ind->tree->size --;
        return FLOAT_RESULT_ROOT;
    }
    parent = *ref_parent;
    for(int j=0; freq && j<BUF_LEN; j++){
        key_i = atomic_load((uint64_t*)parent->buffer[j].key);
        if(key_i == EMPTY_FLAG){
            *(uint64_t*)parent->buffer[j].value = VALUE_TO_STORE(update_value, value_ptr);
            *(uint64_t*)parent->buffer[j].key = *(uint64_t*)key;
            *key_ptr = EMPTY_FLAG;
            if(IS_LEAF(n)){
                delta_bcount = remove_child(parent, ref_parent, key[depth_parent], ref_n, depth_parent);
                if(1 - delta_bcount)
                    ind->tree->buffer_count += 1 - delta_bcount;
            }
            return FLOAT_RESULT_MOVE;
        }
        if(key_i == OCCUPIED_FLAG && rand() % BUF_LEN) 
            continue;
        f = countmin_count(ind->cm, &key_i, KEY_LEN);
        f = countmin_amplify(f); 
        if(freq > f || (freq == f && memcmp(key + depth_parent, (uint8_t*)&key_i + depth_parent, depth - depth_parent) == 0)){
            key_lru = key_i;
            entry_lru = parent->buffer + j;
            break;
        }
    }
    if(key_lru != EMPTY_FLAG){  
        if(memcmp(key + depth_parent, (uint8_t*)&key_lru + depth_parent, depth - depth_parent)) { // Have to reinsert
            val = atomic_exchange((uint64_t*)entry_lru->value, VALUE_TO_STORE(update_value, value_ptr));
            *(uint64_t*)entry_lru->key = *(uint64_t*)key;
            *key_ptr = EMPTY_FLAG;
            if(IS_LEAF(n))
                delta_bcount = remove_child(parent, ref_parent, key[depth_parent], ref_n, depth_parent);
            f = 0;
            // Note that here using *ref_parent is a must, because the remove_child above may replace the node, making parent invalid.
            recursive_insert(*ref_parent, ref_parent, (uint8_t*)&key_lru, KEY_LEN | MEM_TYPE, (void*)&val, depth_parent, &f, 0);
            delta_bcount = INSERT_IS_IN_BUFFER(f) - !IS_LEAF(n) - delta_bcount;
            if(delta_bcount)
                ind->tree->buffer_count += delta_bcount;
            return FLOAT_RESULT_REINSERT;
        }
        else{ // key_lru and key match, so we can directly swap both of them.
            val = atomic_exchange((uint64_t*)entry_lru->value, VALUE_TO_STORE(update_value, value_ptr));
            *(uint64_t*)entry_lru->key = *(uint64_t*)key;
            *value_ptr = val;
            *key_ptr = key_lru;
            return FLOAT_RESULT_SWAP;
        }
    }
    return 0;
};
#endif

int art_search(index_sys *ind, const unsigned char *key, uint64_t* h, uint64_t* query_result) {
    art_node **child = & ind->tree->root, **ref_parent = NULL;
    art_node *n = ind->tree->root, *parent = NULL;
    int prefix_len, depth = 0, depth_parent = 0, freq;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = (art_node*)LEAF_RAW(n);
            // Check if the expanded path matches
            if (!leaf_matches((art_leaf*)n, key, KEY_LEN, depth)) {
                *query_result = atomic_load((uint64_t*)((art_leaf*)n)->value);
                #if SDR_FLOAT & 2
                if((ind->sample_acc & (CM_SAMPLE_INTERVAL*FLOAT_AMP-1)) == 0){
                    freq = countmin_inc_explicit(ind->cm, key, KEY_LEN, h);
                    art_float(ind, key, PACK_INFO(freq, 0, depth, depth_parent), child, ref_parent, NULL);
                }
                #endif // SDR_FLOAT_LEAF
                return 1;
            }
            return 0;
        }

        // Bail if the prefix does not match
        if (n->partial_len) {
            prefix_len = check_prefix(n, key, KEY_LEN, depth);
            if (prefix_len != min(MAX_PREFIX_LEN, n->partial_len))
                return 0;
            depth = depth + n->partial_len;
        }
        #ifdef BUF_LEN
        // Don't go down so soon yet! let's check the buffer!
        for(int i=0; i<BUF_LEN; i++){
            if(atomic_load((uint64_t*)n->buffer[i].key) == *(uint64_t*)key){
                *query_result = atomic_load((uint64_t*)n->buffer[i].value);
                #if SDR_FLOAT & 1
                if((ind->sample_acc & (CM_SAMPLE_INTERVAL*FLOAT_AMP-1)) == 0){
                    freq = countmin_inc_explicit(ind->cm, key, KEY_LEN, h);
                    art_float(ind, key, PACK_INFO(freq, i, depth, depth_parent), child, ref_parent, NULL);
                }
                #endif // SDR_FLOAT
                return 1;
            }
        }
        #endif // BUF_LEN
        ref_parent = child;
        parent = n;
        depth_parent = depth;
        // Recursively search
        child = find_child(n, key[depth]);
        n = (child) ? *child : NULL;
        depth++;
    }
    return 0;
}

void* art_update(index_sys *ind, const unsigned char *key, const int freq, void* value) {
    art_node **child = & ind->tree->root, **ref_parent = NULL;
    art_node *n = ind->tree->root, *parent = NULL;
    int prefix_len, depth = 0, depth_parent = 0, f;
    uint64_t val;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = (art_node*)LEAF_RAW(n);
            // Check if the expanded path matches
            if (!leaf_matches((art_leaf*)n, key, KEY_LEN, depth)) {
                val = atomic_exchange((uint64_t*)((art_leaf*)n)->value, *(uint64_t*)value);
                #if SDR_FLOAT & 2
                if((ind->sample_acc & (CM_SAMPLE_INTERVAL*FLOAT_AMP-1)) == 0)
                    art_float(ind, key, PACK_INFO(freq, 0, depth, depth_parent), child, ref_parent, value);
                #endif
                return (void*)val;
            }
            return NULL;
        }

        // Bail if the prefix does not match
        if (n->partial_len) {
            prefix_len = check_prefix(n, key, KEY_LEN, depth);
            if (prefix_len != min(MAX_PREFIX_LEN, n->partial_len))
                return NULL;
            depth = depth + n->partial_len;
        }
        #ifdef BUF_LEN
        // Don't go down so soon yet! let's check the buffer!
        for(int i=0; i<BUF_LEN; i++){
            if(atomic_load((uint64_t*)n->buffer[i].key) == *(uint64_t*)key){
                void* old = n->buffer[i].value;
                #if SDR_FLOAT & 1
                if((ind->sample_acc & (CM_SAMPLE_INTERVAL*FLOAT_AMP-1)) == 0)
                    if(art_float(ind, key, PACK_INFO(freq, i, depth, depth_parent), child, ref_parent, value)){
                        return old;
                    }
                #endif
                *(uint64_t*)n->buffer[i].value = *(uint64_t*)value;
                return old;
            }
        }
        #endif
        ref_parent = child;
        parent = n;
        depth_parent = depth;
        // Recursively search
        child = find_child(n, key[depth]);
        n = (child) ? *child : NULL;
        depth++;
    }
    return NULL;
}

// Find the minimum leaf under a node
static art_leaf* minimum(const art_node *n) {
    // Handle base cases
    if (!n) return NULL;
    if (IS_LEAF(n)) return LEAF_RAW(n);

    int idx;
    switch (n->type) {
        case NODE4:
            return minimum(((const art_node4*)n)->children[0]);
        case NODE16:
            return minimum(((const art_node16*)n)->children[0]);
        case NODE48:
            idx=0;
            while (!((const art_node48*)n)->keys[idx]) idx++;
            idx = ((const art_node48*)n)->keys[idx] - 1;
            return minimum(((const art_node48*)n)->children[idx]);
        case NODE256:
            idx=0;
            while (!((const art_node256*)n)->children[idx]) idx++;
            return minimum(((const art_node256*)n)->children[idx]);
        default:
            abort();
    }
}

// Find the maximum leaf under a node
static art_leaf* maximum(const art_node *n) {
    // Handle base cases
    if (!n) return NULL;
    if (IS_LEAF(n)) return LEAF_RAW(n);

    int idx;
    switch (n->type) {
        case NODE4:
            return maximum(((const art_node4*)n)->children[n->num_children-1]);
        case NODE16:
            return maximum(((const art_node16*)n)->children[n->num_children-1]);
        case NODE48:
            idx=255;
            while (!((const art_node48*)n)->keys[idx]) idx--;
            idx = ((const art_node48*)n)->keys[idx] - 1;
            return maximum(((const art_node48*)n)->children[idx]);
        case NODE256:
            idx=255;
            while (!((const art_node256*)n)->children[idx]) idx--;
            return maximum(((const art_node256*)n)->children[idx]);
        default:
            abort();
    }
}

art_leaf* art_minimum(art_tree *t) {
    return minimum((art_node*)t->root);
}

art_leaf* art_maximum(art_tree *t) {
    return maximum((art_node*)t->root);
}

static art_leaf* make_leaf(const unsigned char *key, int key_len, void *value) {
    art_leaf *l = (art_leaf*)calloc(1, sizeof(art_leaf));
    *(uint64_t*)l->value = *(uint64_t*)value;
    *(uint64_t*)l->key = *(uint64_t*)key;
    //memcpy(l->key, key, key_len);
    return l;
}

static int longest_common_prefix(art_leaf *l1, art_leaf *l2, int depth) {
    int max_cmp = KEY_LEN - depth;
    int idx;
    for (idx=0; idx < max_cmp; idx++) {
        if (l1->key[depth+idx] != l2->key[depth+idx])
            return idx;
    }
    return idx;
}

static void copy_header(art_node *dest, art_node *src) {
    dest->num_children = src->num_children;
    dest->partial_len = src->partial_len;
    #ifdef BUF_LEN
    memcpy(dest->buffer, src->buffer, sizeof(entry)*BUF_LEN);
    #endif
    memcpy(dest->partial, src->partial, min(MAX_PREFIX_LEN, src->partial_len));
}

static void add_child256(art_node256 *n, art_node **ref, unsigned char c, void *child) {
    (void)ref;
    n->n.num_children++;
    n->children[c] = (art_node*)child;
}

static void add_child48(art_node48 *n, art_node **ref, unsigned char c, void *child) {
    if (n->n.num_children < 48) {
        int pos = 0;
        while (n->children[pos]) pos++;
        n->children[pos] = (art_node*)child;
        n->keys[c] = pos + 1;
        n->n.num_children++;
    } else {
        art_node256 *new_node = (art_node256*)alloc_node(NODE256);
        for (int i=0;i<256;i++) {
            if (n->keys[i]) {
                new_node->children[i] = n->children[n->keys[i] - 1];
            }
        }
        copy_header((art_node*)new_node, (art_node*)n);
        *ref = (art_node*)new_node;
        free(n);
        add_child256(new_node, ref, c, child);
    }
}

static void add_child16(art_node16 *n, art_node **ref, unsigned char c, void *child) {
    if (n->n.num_children < 16) {
        unsigned mask = (1 << n->n.num_children) - 1;
        
        // support non-x86 architectures
        #ifdef __i386__
            __m128i cmp;

            // Compare the key to all 16 stored keys
            cmp = _mm_cmplt_epi8(_mm_set1_epi8(c),
                    _mm_loadu_si128((__m128i*)n->keys));

            // Use a mask to ignore children that don't exist
            unsigned bitfield = _mm_movemask_epi8(cmp) & mask;
        #else
        #ifdef __amd64__
            __m128i cmp;

            // Compare the key to all 16 stored keys
            cmp = _mm_cmplt_epi8(_mm_set1_epi8(c),
                    _mm_loadu_si128((__m128i*)n->keys));

            // Use a mask to ignore children that don't exist
            unsigned bitfield = _mm_movemask_epi8(cmp) & mask;
        #else
            // Compare the key to all 16 stored keys
            unsigned bitfield = 0;
            for (short i = 0; i < 16; ++i) {
                if (c < n->keys[i])
                    bitfield |= (1 << i);
            }

            // Use a mask to ignore children that don't exist
            bitfield &= mask;    
        #endif
        #endif

        // Check if less than any
        unsigned idx;
        if (bitfield) {
            idx = __builtin_ctz(bitfield);
            memmove(n->keys+idx+1,n->keys+idx,n->n.num_children-idx);
            memmove(n->children+idx+1,n->children+idx,
                    (n->n.num_children-idx)*sizeof(void*));
        } else
            idx = n->n.num_children;

        // Set the child
        n->keys[idx] = c;
        n->children[idx] = (art_node*)child;
        n->n.num_children++;

    } else {
        art_node48 *new_node = (art_node48*)alloc_node(NODE48);

        // Copy the child pointers and populate the key map
        memcpy(new_node->children, n->children,
                sizeof(void*)*n->n.num_children);
        for (int i=0;i<n->n.num_children;i++) {
            new_node->keys[n->keys[i]] = i + 1;
        }
        copy_header((art_node*)new_node, (art_node*)n);
        *ref = (art_node*)new_node;
        free(n);
        add_child48(new_node, ref, c, child);
    }
}

static void add_child4(art_node4 *n, art_node **ref, unsigned char c, void *child) {
    if (n->n.num_children < 4) {
        int idx;
        for (idx=0; idx < n->n.num_children; idx++) {
            if (c < n->keys[idx]) break;
        }

        // Shift to make room
        memmove(n->keys+idx+1, n->keys+idx, n->n.num_children - idx);
        memmove(n->children+idx+1, n->children+idx,
                (n->n.num_children - idx)*sizeof(void*));

        // Insert element
        n->keys[idx] = c;
        n->children[idx] = (art_node*)child;
        n->n.num_children++;

    } else {
        art_node16 *new_node = (art_node16*)alloc_node(NODE16);

        // Copy the child pointers and the key map
        memcpy(new_node->children, n->children,
                sizeof(void*)*n->n.num_children);
        memcpy(new_node->keys, n->keys,
                sizeof(unsigned char)*n->n.num_children);
        copy_header((art_node*)new_node, (art_node*)n);
        *ref = (art_node*)new_node;
        free(n);
        add_child16(new_node, ref, c, child);
    }
}

static void add_child(art_node *n, art_node **ref, unsigned char c, void *child) {
    switch (n->type) {
        case NODE4:
            return add_child4((art_node4*)n, ref, c, child);
        case NODE16:
            return add_child16((art_node16*)n, ref, c, child);
        case NODE48:
            return add_child48((art_node48*)n, ref, c, child);
        case NODE256:
            return add_child256((art_node256*)n, ref, c, child);
        default:
            abort();
    }
}

/**
 * Calculates the index at which the prefixes mismatch
 */
static int prefix_mismatch(const art_node *n, const unsigned char *key, int key_len, int depth) {
    int max_cmp = min(min(MAX_PREFIX_LEN, n->partial_len), key_len - depth);
    int idx;
    for (idx=0; idx < max_cmp; idx++) {
        if (n->partial[idx] != key[depth+idx])
            return idx;
    }

    // If the prefix is short we can avoid finding a leaf
    if (n->partial_len > MAX_PREFIX_LEN) {
        // Prefix is longer than what we've checked, find a leaf
        art_leaf *l = minimum(n);
        max_cmp = KEY_LEN - depth;
        for (; idx < max_cmp; idx++) {
            if (l->key[idx+depth] != key[depth+idx])
                return idx;
        }
    }
    return idx;
}

static void* recursive_insert(art_node *n, art_node **ref, const unsigned char *key, int key_len, void *value, int depth, int *old, int replace) {
    int storage_type = key_len & 0x7;
    key_len &= ~0x7;
    uint64_t comp;
    // If we are at a NULL node, inject a leaf
    if (!n) {
        *ref = (art_node*)SET_LEAF(make_leaf(key, key_len, value));
        return NULL;
    }

    // If we are at a leaf, we need to replace it with a node
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);

        // Check if we are updating an existing value
        if (!leaf_matches(l, key, key_len, depth)) {
            *old = INSERT_FOUND_KEY_FLAG;
            void *old_val = l->value;
            if(replace) *(uint64_t*)l->value = *(uint64_t*)value;
            return old_val;
        }

        // New value, we must split the leaf into a node4
        art_node4 *new_node = (art_node4*)alloc_node(NODE4);

        // Create a new leaf
        art_leaf *l2 = make_leaf(key, key_len, value);

        // Determine longest prefix
        int longest_prefix = longest_common_prefix(l, l2, depth);
        new_node->n.partial_len = longest_prefix;
        memcpy(new_node->n.partial, key+depth, min(MAX_PREFIX_LEN, longest_prefix));
        // Add the leafs to the new node4
        add_child4(new_node, ref, l->key[depth+longest_prefix], SET_LEAF(l));
        add_child4(new_node, ref, l2->key[depth+longest_prefix], SET_LEAF(l2));
        *ref = (art_node*)new_node;
        return NULL;
    }

    // TODO: We want to put MEM_TYPE in buffer, so we don't have to go all the way down the tree
    #ifdef BUF_LEN
    if(storage_type == MEM_TYPE){
        int z = -1;
        for(int i=0; i<BUF_LEN; i++){
            if(atomic_load((uint64_t*)n->buffer[i].key)==0) 
                z = i;
            if(atomic_load((uint64_t*)n->buffer[i].key)==*(uint64_t*)key){
                void *old_val = n->buffer[i].value;
                *old = INSERT_FOUND_KEY_FLAG;
                if(replace) *(uint64_t*)n->buffer[i].value = *(uint64_t*)value;
                return old_val;
            }
        }
        if(!replace && z != -1)
            for(int i=z; i<BUF_LEN; i++){
                comp = 0;
                if(atomic_compare_exchange_strong((uint64_t*)n->buffer[i].key, &comp, *(uint64_t*)key)){
                    *(uint64_t*)n->buffer[i].value = *(uint64_t*)value;
                    *old = INSERT_IS_IN_BUFFER_FLAG;
                    return NULL;
                }
            }
    }
    #endif

    // Check if given node has a prefix
    if (n->partial_len) {
        // Determine if the prefixes differ, since we need to split
        int prefix_diff = prefix_mismatch(n, key, key_len, depth);
        if ((uint32_t)prefix_diff >= n->partial_len) {
            depth += n->partial_len;
            goto RECURSIVE_SEARCH;
        }

        // Create a new node
        art_node4 *new_node = (art_node4*)alloc_node(NODE4);
        
        new_node->n.partial_len = prefix_diff;
        memcpy(new_node->n.partial, n->partial, min(MAX_PREFIX_LEN, prefix_diff));
        #ifdef BUF_LEN
        memcpy(new_node->n.buffer, n->buffer, sizeof(entry)*BUF_LEN);
        memset(n->buffer, 0, sizeof(entry)*BUF_LEN);
        #endif

        // Adjust the prefix of the old node
        if (n->partial_len <= MAX_PREFIX_LEN) {
            add_child4(new_node, (art_node**)&new_node, n->partial[prefix_diff], n);
            n->partial_len -= (prefix_diff+1);
            memmove(n->partial, n->partial+prefix_diff+1,
                    min(MAX_PREFIX_LEN, n->partial_len));
        } else {
            n->partial_len -= (prefix_diff+1);
            art_leaf *l = minimum(n);
            add_child4(new_node, (art_node**)&new_node, l->key[depth+prefix_diff], n);
            memcpy(n->partial, l->key+depth+prefix_diff+1,
                    min(MAX_PREFIX_LEN, n->partial_len));
        }

        // Insert the new leaf
        art_leaf *l = make_leaf(key, key_len, value);
        add_child4(new_node, (art_node**)&new_node, key[depth+prefix_diff], SET_LEAF(l));
        *ref = (art_node*)new_node;
        return NULL;
    }

RECURSIVE_SEARCH:;

    // Find a child to recurse to
    art_node **child = find_child(n, key[depth]);
    if (child) {
        return recursive_insert(*child, child, key, key_len | storage_type, value, depth+1, old, replace);
    }

    // No child, node goes within us
    art_leaf *l = make_leaf(key, key_len, value);
    add_child(n, ref, key[depth], SET_LEAF(l));
    return NULL;
}

void* art_insert(art_tree *t, const unsigned char *key, int key_len, void *value) {
    int old_val = 0;
    void *old = recursive_insert(t->root, &t->root, key, key_len, value, 0, &old_val, 1);
    if (INSERT_IS_IN_BUFFER(old_val)){
        old_val ^= INSERT_IS_IN_BUFFER_FLAG;
        t->buffer_count++;
    }
    if (!old_val) t->size++;
    return old;
}

void* art_insert_no_replace(art_tree *t, const unsigned char *key, int key_len, void *value) {
    int old_val = 0;
    void *old = recursive_insert(t->root, &t->root, key, key_len, value, 0, &old_val, 0);
    if (INSERT_IS_IN_BUFFER(old_val)){
        old_val ^= INSERT_IS_IN_BUFFER_FLAG;
        t->buffer_count++;
    }
    if (!old_val) t->size++;
    return old;
}

static int remove_child256(art_node256 *n, art_node **ref, unsigned char c) {
    n->children[c] = NULL;
    n->n.num_children--;

    // Resize to a node48 on underflow, not immediately to prevent
    // trashing if we sit on the 48/49 boundary
    if (n->n.num_children == 37) {
        art_node48 *new_node = (art_node48*)alloc_node(NODE48);
        *ref = (art_node*)new_node;
        copy_header((art_node*)new_node, (art_node*)n);

        int pos = 0;
        for (int i=0;i<256;i++) {
            if (n->children[i]) {
                new_node->children[pos] = n->children[i];
                new_node->keys[i] = pos + 1;
                pos++;
            }
        }
        free(n);
    }
    return 0;
}

static int remove_child48(art_node48 *n, art_node **ref, unsigned char c) {
    int pos = n->keys[c];
    n->keys[c] = 0;
    n->children[pos-1] = NULL;
    n->n.num_children--;

    if (n->n.num_children == 12) {
        art_node16 *new_node = (art_node16*)alloc_node(NODE16);
        *ref = (art_node*)new_node;
        copy_header((art_node*)new_node, (art_node*)n);

        int child = 0;
        for (int i=0;i<256;i++) {
            pos = n->keys[i];
            if (pos) {
                new_node->keys[child] = i;
                new_node->children[child] = n->children[pos - 1];
                child++;
            }
        }
        free(n);
    }
    return 0;
}

static int remove_child16(art_node16 *n, art_node **ref, art_node **l) {
    int pos = l - n->children;
    memmove(n->keys+pos, n->keys+pos+1, n->n.num_children - 1 - pos);
    memmove(n->children+pos, n->children+pos+1, (n->n.num_children - 1 - pos)*sizeof(void*));
    n->n.num_children--;

    if (n->n.num_children == 3) {
        art_node4 *new_node = (art_node4*)alloc_node(NODE4);
        *ref = (art_node*)new_node;
        copy_header((art_node*)new_node, (art_node*)n);
        memcpy(new_node->keys, n->keys, 4);
        memcpy(new_node->children, n->children, 4*sizeof(void*));
        free(n);
    }
    return 0;
}

static int remove_child4(art_node4 *n, art_node **ref, art_node **l, int depth) {
    int pos = l - n->children, cnt = 0;
    memmove(n->keys+pos, n->keys+pos+1, n->n.num_children - 1 - pos);
    memmove(n->children+pos, n->children+pos+1, (n->n.num_children - 1 - pos)*sizeof(void*));
    n->n.num_children--;
    #ifdef BUF_LEN
    for(int i=0; i<BUF_LEN; i++){
        if(n->n.num_children < 4 && !IS_SPECIAL_KEY(n->n.buffer[i].key)){
            art_leaf *k = make_leaf(n->n.buffer[i].key, KEY_LEN, n->n.buffer[i].value);
            add_child4(n, ref, n->n.buffer[i].key[depth], SET_LEAF(k));
            *(uint64_t*)n->n.buffer[i].key = EMPTY_FLAG;
            cnt++;
        }
    }
    if(cnt)
        return cnt;
    #endif
    // Remove nodes with only a single child
    if (n->n.num_children == 1) {
        art_node *child = n->children[0];
        if (!IS_LEAF(child)) {
            // Concatenate the prefixes
            int prefix = n->n.partial_len;
            if (prefix < MAX_PREFIX_LEN) {
                n->n.partial[prefix] = n->keys[0];
                prefix++;
            }
            if (prefix < MAX_PREFIX_LEN) {
                int sub_prefix = min(child->partial_len, MAX_PREFIX_LEN - prefix);
                memcpy(n->n.partial+prefix, child->partial, sub_prefix);
                prefix += sub_prefix;
            }

            // Store the prefix in the child
            memcpy(child->partial, n->n.partial, min(prefix, MAX_PREFIX_LEN));
            child->partial_len += n->n.partial_len + 1;
        }
        *ref = child;
        free(n);
    }
    return 0;
}

static int remove_child(art_node *n, art_node **ref, unsigned char c, art_node **l, int depth) {
    switch (n->type) {
        case NODE4:
            return remove_child4((art_node4*)n, ref, l, depth);
        case NODE16:
            return remove_child16((art_node16*)n, ref, l);
        case NODE48:
            return remove_child48((art_node48*)n, ref, c);
        case NODE256:
            return remove_child256((art_node256*)n, ref, c);
        default:
            abort();
    }
}

static void* recursive_delete(art_node *n, art_node **ref, const unsigned char *key, int key_len, int depth) {
    // Search terminated
    if (!n) return NULL;

    void *old = NULL;

    // Handle hitting a leaf node
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);
        if (!leaf_matches(l, key, key_len, depth)) {
            *ref = NULL;
            old = l->value;
            free(l);
            return old;
        }
        return NULL;
    }

    // Bail if the prefix does not match
    if (n->partial_len) {
        int prefix_len = check_prefix(n, key, key_len, depth);
        if (prefix_len != min(MAX_PREFIX_LEN, n->partial_len)) {
            return NULL;
        }
        depth = depth + n->partial_len;
    }

    #ifdef BUF_LEN
    // Don't go down so soon yet! let's check the buffer!
    for(int i=0; i<BUF_LEN; i++){
        if(*(uint64_t*)n->buffer[i].key == *(uint64_t*)key){
            *(uint64_t*)n->buffer[i].key = EMPTY_FLAG;
            return (void*)((uintptr_t)n->buffer[i].value | 1);
        }
    }
    #endif

    // Find child node
    art_node **child = find_child(n, key[depth]);
    if (!child) return NULL;

    // If the child is leaf, delete from this node
    if (IS_LEAF(*child)) {
        art_leaf *l = LEAF_RAW(*child);
        if (!leaf_matches(l, key, key_len, depth)) {
            old = l->value;
            old = (void*)((uintptr_t)old | remove_child(n, ref, key[depth], child, depth));
            free(l);
            return old;
        }
        return NULL;

    // Recurse
    } else {
        return recursive_delete(*child, child, key, key_len, depth+1);
    }
}

void* art_delete(art_tree *t, const unsigned char *key, int key_len) {
    void *old = recursive_delete(t->root, &t->root, key, key_len, 0);
    if ((uintptr_t)old & 0x7){
        t->buffer_count -= (uintptr_t)old & 0x7;
        old = (void*)((uintptr_t)old & ~7);
    }
    if (old) {
        t->size--;
        return old;
    }
    return NULL;
}

// Recursively iterates over the tree
static int recursive_iter(art_node *n, art_callback cb, void *data) {
    // Handle base cases
    if (!n) return 0;
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);
        return cb(data, (const unsigned char*)l->key, KEY_LEN, l->value);
    }

    int idx, res;
    switch (n->type) {
        case NODE4:
            for (int i=0; i < n->num_children; i++) {
                res = recursive_iter(((art_node4*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        case NODE16:
            for (int i=0; i < n->num_children; i++) {
                res = recursive_iter(((art_node16*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        case NODE48:
            for (int i=0; i < 256; i++) {
                idx = ((art_node48*)n)->keys[i];
                if (!idx) continue;

                res = recursive_iter(((art_node48*)n)->children[idx-1], cb, data);
                if (res) return res;
            }
            break;

        case NODE256:
            for (int i=0; i < 256; i++) {
                if (!((art_node256*)n)->children[i]) continue;
                res = recursive_iter(((art_node256*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        default:
            abort();
    }
    return 0;
}

int art_iter(art_tree *t, art_callback cb, void *data) {
    return recursive_iter(t->root, cb, data);
}

/**
 * Checks if a leaf prefix matches
 * @return 0 on success.
 */
static int leaf_prefix_matches(const art_leaf *n, const unsigned char *prefix, int prefix_len) {
    // Fail if the key length is too short
    if (KEY_LEN < (uint32_t)prefix_len) return 1;

    // Compare the keys
    return memcmp(n->key, prefix, prefix_len);
}

int art_iter_prefix(art_tree *t, const unsigned char *key, int key_len, art_callback cb, void *data) {
    art_node **child;
    art_node *n = t->root;
    int prefix_len, depth = 0;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = (art_node*)LEAF_RAW(n);
            // Check if the expanded path matches
            if (!leaf_prefix_matches((art_leaf*)n, key, key_len)) {
                art_leaf *l = (art_leaf*)n;
                return cb(data, (const unsigned char*)l->key, KEY_LEN, l->value);
            }
            return 0;
        }

        // If the depth matches the prefix, we need to handle this node
        if (depth == key_len) {
            art_leaf *l = minimum(n);
            if (!leaf_prefix_matches(l, key, key_len))
               return recursive_iter(n, cb, data);
            return 0;
        }

        // Bail if the prefix does not match
        if (n->partial_len) {
            prefix_len = prefix_mismatch(n, key, key_len, depth);

            // Guard if the mis-match is longer than the MAX_PREFIX_LEN
            if ((uint32_t)prefix_len > n->partial_len) {
                prefix_len = n->partial_len;
            }

            // If there is no match, search is terminated
            if (!prefix_len) {
                return 0;

            // If we've matched the prefix, iterate on this node
            } else if (depth + prefix_len == key_len) {
                return recursive_iter(n, cb, data);
            }

            // if there is a full match, go deeper
            depth = depth + n->partial_len;
        }

        // Recursively search
        child = find_child(n, key[depth]);
        n = (child) ? *child : NULL;
        depth++;
    }
    return 0;
}