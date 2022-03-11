#include <stdint.h>
#include "common.h"
#ifndef ART_H
#define ART_H
// Code adapted from https://github.com/armon/libart
__BEGIN_DECLS

#if defined(__GNUC__) && !defined(__clang__)
# if __STDC_VERSION__ >= 199901L && 402 == (__GNUC__ * 100 + __GNUC_MINOR__)
/*
 * GCC 4.2.2's C99 inline keyword support is pretty broken; avoid. Introduced in
 * GCC 4.2.something, fixed in 4.3.0. So checking for specific major.minor of
 * 4.2 is fine.
 */
#  define BROKEN_GCC_C99_INLINE
# endif
#endif

typedef int(*art_callback)(void *data, const unsigned char *key, uint32_t key_len, void *value);


/**
 * Initializes an ART tree
 * @return 0 on success.
 */
int art_tree_init(art_tree *t);

/**
 * DEPRECATED
 * Initializes an ART tree
 * @return 0 on success.
 */
#define init_art_tree(...) art_tree_init(__VA_ARGS__)

/**
 * Destroys an ART tree
 * @return 0 on success.
 */
int art_tree_destroy(art_tree *t);

/**
 * DEPRECATED
 * Initializes an ART tree
 * @return 0 on success.
 */
#define destroy_art_tree(...) art_tree_destroy(__VA_ARGS__)

/**
 * Returns the size of the ART tree.
 */
#ifdef BROKEN_GCC_C99_INLINE
# define art_size(t) ((t)->size)
#else
inline uint64_t art_size(art_tree *t) {
    return t->size;
}
#endif

/**
 * inserts a new value into the art tree
 * @param t the tree
 * @param key the key
 * @param key_len the length of the key
 * @param value opaque value.
 * @return null if the item was newly inserted, otherwise
 * the old value pointer is returned.
 */
void* art_insert(art_tree *t, const unsigned char *key, int key_len, void *value);

/**
 * inserts a new value into the art tree (not replacing)
 * @param t the tree
 * @param key the key
 * @param key_len the length of the key
 * @param value opaque value.
 * @return null if the item was newly inserted, otherwise
 * the old value pointer is returned.
 */
void* art_insert_no_replace(art_tree *t, const unsigned char *key, int key_len, void *value);

/**
 * Searches for a value in the ART tree, update it inplace and returns old value
 * @param t The index system
 * @param key The key
 * @param freq Estimated frequency of the key to query. 
 *              This parameter will be used to weight the importance of the data and place the data in INB accordingly, 
 *              so you can also manually set a very high value if you want to place the data at very top,
 *              or just give a 0 TO DISABLE the internal smart data re-placing mechanism.
 * @param value opaque value.
 * @param cm The Count-Min Sketch for estimating frequency of keys
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void* art_update(index_sys *ind, const unsigned char *key, const int freq, void* value);

/**
 * Deletes a value from the ART tree
 * @param t The tree
 * @param key The key
 * @param key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void* art_delete(art_tree *t, const unsigned char *key, int key_len);

/**
 * Searches for a value in the ART tree
 * @param ind The index system
 * @param key The key
 * @param freq Estimated frequency of the key to query. 
 *              This parameter will be used to weight the importance of the data and place the data in INB accordingly, 
 *              so you can also manually set a very high value if you want to place the data at very top,
 *              or just give a 0 TO DISABLE the internal smart data re-placing mechanism.
 * @param query_result a pointer to store query result.
 * @param cm The Count-Min Sketch for estimating frequency of other keys
 * @return 1 if the item was found, 0 otherwise
 * the value pointer is returned.
 */
int art_search(index_sys *ind, const unsigned char *key, const int freq, uint64_t* query_result);

/**
 * Returns the minimum valued leaf
 * @return The minimum leaf or NULL
 */
art_leaf* art_minimum(art_tree *t);

/**
 * Returns the maximum valued leaf
 * @return The maximum leaf or NULL
 */
art_leaf* art_maximum(art_tree *t);

/**
 * Iterates through the entries pairs in the map,
 * invoking a callback for each. The call back gets a
 * key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @param t The tree to iterate over
 * @param cb The callback function to invoke
 * @param data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int art_iter(art_tree *t, art_callback cb, void *data);

/**
 * Iterates through the entries pairs in the map,
 * invoking a callback for each that matches a given prefix.
 * The call back gets a key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @param t The tree to iterate over
 * @param prefix The prefix of keys to read
 * @param prefix_len The length of the prefix
 * @param cb The callback function to invoke
 * @param data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int art_iter_prefix(art_tree *t, const unsigned char *prefix, int prefix_len, art_callback cb, void *data);

__END_DECLS

#endif