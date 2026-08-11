#include "hashmap.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct hashmap_entry {
  char *key;
  char *value;

  struct hashmap_entry *next;
} hashmap_entry_t;

struct hashmap {
  hashmap_entry_t **buckets;
  size_t capacity;
  size_t size;
};

#define HASHMAP_INITIAL_CAPACITY 10
#define HASHMAP_MAX_LOAD_FACTOR 0.75

static uint64_t hash_string(const char *key) {
  /*
   * FNV-1a 64-bit hash.
   */
  uint64_t hash = 14695981039346656037ULL;

  while (*key != '\0') {
    hash ^= (unsigned char)*key;
    hash *= 1099511628211ULL;
    key++;
  }

  return hash;
}

static char *string_duplicate(const char *source) {
  size_t length;
  char *copy;

  length = strlen(source) + 1;

  copy = malloc(length);
  if (copy == NULL) {
    return NULL;
  }

  memcpy(copy, source, length);

  return copy;
}

static hashmap_entry_t *entry_create(const char *key, const char *value) {
  hashmap_entry_t *entry;

  entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    return NULL;
  }

  entry->key = string_duplicate(key);
  if (entry->key == NULL) {
    free(entry);
    return NULL;
  }

  entry->value = string_duplicate(value);
  if (entry->value == NULL) {
    free(entry->key);
    free(entry);
    return NULL;
  }

  entry->next = NULL;

  return entry;
}

static void entry_destroy(hashmap_entry_t *entry) {
  if (entry == NULL) {
    return;
  }

  free(entry->key);
  free(entry->value);
  free(entry);
}

static bool hashmap_resize(hashmap_t *map, size_t new_capacity) {
  hashmap_entry_t **new_buckets;

  new_buckets = calloc(new_capacity, sizeof(*new_buckets));

  if (new_buckets == NULL) {
    return false;
  }

  for (size_t i = 0; i < map->capacity; i++) {
    hashmap_entry_t *entry = map->buckets[i];

    while (entry != NULL) {
      hashmap_entry_t *next = entry->next;

      size_t index = hash_string(entry->key) % new_capacity;

      entry->next = new_buckets[index];
      new_buckets[index] = entry;

      entry = next;
    }
  }

  free(map->buckets);

  map->buckets = new_buckets;
  map->capacity = new_capacity;

  return true;
}

// ------------------------------------------------------------------------------

hashmap_t *hashmap_create(size_t initial_capacity) {
  hashmap_t *map;

  if (initial_capacity == 0) {
    initial_capacity = HASHMAP_INITIAL_CAPACITY;
  }

  map = malloc(sizeof(*map));
  if (map == NULL) {
    return NULL;
  }

  map->buckets = calloc(initial_capacity, sizeof(*map->buckets));

  if (map->buckets == NULL) {
    free(map);
    return NULL;
  }

  map->capacity = initial_capacity;
  map->size = 0;

  return map;
}

void hashmap_destroy(hashmap_t *map) {
  if (map == NULL) {
    return;
  }

  for (size_t i = 0; i < map->capacity; i++) {
    hashmap_entry_t *entry = map->buckets[i];

    while (entry != NULL) {
      hashmap_entry_t *next = entry->next;

      entry_destroy(entry);
      entry = next;
    }
  }

  free(map->buckets);
  free(map);
}

bool hashmap_set(hashmap_t *map, const char *key, const char *value) {
  size_t index;
  hashmap_entry_t *entry;

  if (map == NULL || key == NULL || value == NULL) {
    return false;
  }

  index = hash_string(key) % map->capacity;

  entry = map->buckets[index];

  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      char *new_value = string_duplicate(value);

      if (new_value == NULL) {
        return false;
      }

      free(entry->value);
      entry->value = new_value;

      return true;
    }

    entry = entry->next;
  }

  entry = entry_create(key, value);

  if (entry == NULL) {
    return false;
  }

  entry->next = map->buckets[index];
  map->buckets[index] = entry;
  map->size++;

  if ((double)map->size / (double)map->capacity > HASHMAP_MAX_LOAD_FACTOR) {

    /*
     * Resizing failure does not invalidate the
     * insertion that already succeeded.
     */
    (void)hashmap_resize(map, map->capacity * 2);
  }

  return true;
}

const char *hashmap_get(const hashmap_t *map, const char *key) {
  size_t index;
  hashmap_entry_t *entry;

  if (map == NULL || key == NULL) {
    return NULL;
  }

  index = hash_string(key) % map->capacity;

  entry = map->buckets[index];

  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      return entry->value;
    }

    entry = entry->next;
  }

  return NULL;
}

bool hashmap_delete(hashmap_t *map, const char *key) {
  size_t index;
  hashmap_entry_t *entry;
  hashmap_entry_t *previous = NULL;

  if (map == NULL || key == NULL) {
    return false;
  }

  index = hash_string(key) % map->capacity;

  entry = map->buckets[index];

  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      if (previous == NULL) {
        map->buckets[index] = entry->next;
      } else {
        previous->next = entry->next;
      }

      entry_destroy(entry);
      map->size--;

      return true;
    }

    previous = entry;
    entry = entry->next;
  }

  return false;
}

bool hashmap_contains(const hashmap_t *map, const char *key) {
  return hashmap_get(map, key) != NULL;
}

size_t hashmap_size(const hashmap_t *map) {
  if (map == NULL) {
    return 0;
  }

  return map->size;
}
