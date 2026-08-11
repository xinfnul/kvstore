#ifndef KVSTORE_HASHMAP_H
#define KVSTORE_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>

typedef struct hashmap hashmap_t;

/*
 * Create a new hash map.
 *
 * ( initial_capacity must be >= 0 )
 */
hashmap_t *hashmap_create(size_t initial_capacity);

/*
 * Destroy the hash map and all entries stored inside it.
 */
void hashmap_destroy(hashmap_t *map);

/*
 * Insert or update a key/value pair.
 *
 * Returns true on success, false on allocation failure
 * or invalid arguments.
 */
bool hashmap_set(hashmap_t *map, const char *key, const char *value);

/*
 * Retrieve the value associated with the key.
 *
 * Returns NULL is the key does not exist.
 *
 * THe returned pointer belongs to the hashmap and must
 * not be freed by the caller.
 */
const char *hashmap_get(const hashmap_t *map, const char *key);

/*
 * Delete a key/value pair.
 *
 * Returns true if the key existed and was deleted.
 * Returns false is the key did not exist.
 */
bool hashmap_delete(hashmap_t *map, const char *key);

/*
 * Check whether a key exists.
 */
bool hashmap_contains(const hashmap_t *map, const char *key);

/*
 * Return the number of entries currently stored.
 */
size_t hashmap_size(const hashmap_t *map);

#endif
