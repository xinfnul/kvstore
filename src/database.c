#include "database.h"
#include "hashmap.h"

#include <stdlib.h>

struct database {
  hashmap_t *map;
};

database_t *database_create(void) {
  database_t *db = malloc(sizeof(*db));

  if (db == NULL) {
    return NULL;
  }

  db->map = hashmap_create(0);

  if (db->map == NULL) {
    free(db);
    return NULL;
  }

  return db;
}

void database_destroy(database_t *db) {
  if (db == NULL) {
    return;
  }

  hashmap_destroy(db->map);
  free(db);
}

bool database_set(database_t *db, const char *key, const char *value) {
  if (db == NULL || key == NULL || value == NULL) {
    return false;
  }

  return hashmap_set(db->map, key, value);
}

const char *database_get(const database_t *db, const char *key) {
  if (db == NULL || key == NULL) {
    return NULL;
  }

  return hashmap_get(db->map, key);
}

bool database_delete(database_t *db, const char *key) {
  if (db == NULL || key == NULL) {
    return false;
  }

  return hashmap_delete(db->map, key);
}

bool database_contains(const database_t *db, const char *key) {
  if (db == NULL || key == NULL) {
    return false;
  }

  return hashmap_contains(db->map, key);
}

size_t database_size(const database_t *db) {
  if (db == NULL) {
    return 0;
  }

  return hashmap_size(db->map);
}
