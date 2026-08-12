#include <kvstore/database.h>

#include <kvstore/hashmap.h>
#include <kvstore/wal.h>

#include <stdlib.h>
#include <string.h>

struct database {
  hashmap_t *map;
  wal_t *wal;
};

static bool database_replay_callback(wal_operation_t operation, const char *key,
                                     size_t key_length, const char *value,
                                     size_t value_length, void *user_data) {
  database_t *db = user_data;

  if (db == NULL || key == NULL) {
    return false;
  }

  /*
   * The hashmap uses NUL-terminated strings, while WAL records
   * are length-based. Reject embedded NUL bytes so that the
   * replayed data has unambiguous string semantics.
   */
  if (memchr(key, '\0', key_length) != NULL) {
    return false;
  }

  if (operation == WAL_OP_SET) {
    if (value == NULL || memchr(value, '\0', value_length) != NULL) {
      return false;
    }

    char *key_copy = malloc(key_length + 1);
    if (key_copy == NULL) {
      return false;
    }

    char *value_copy = malloc(value_length + 1);
    if (value_copy == NULL) {
      free(key_copy);
      return false;
    }

    memcpy(key_copy, key, key_length);
    key_copy[key_length] = '\0';

    memcpy(value_copy, value, value_length);
    value_copy[value_length] = '\0';

    bool success = hashmap_set(db->map, key_copy, value_copy);

    free(key_copy);
    free(value_copy);

    return success;
  }

  if (operation == WAL_OP_DELETE) {
    char *key_copy = malloc(key_length + 1);
    if (key_copy == NULL) {
      return false;
    }

    memcpy(key_copy, key, key_length);
    key_copy[key_length] = '\0';

    /*
     * A DELETE record is valid even if the key is already
     * absent. This can happen if the WAL contains repeated
     * DELETE operations.
     */
    if (!hashmap_contains(db->map, key_copy)) {
      free(key_copy);
      return true;
    }

    bool success = hashmap_delete(db->map, key_copy);

    free(key_copy);

    return success;
  }

  return false;
}

// ------------------------------------------------------------------------------

database_t *database_open(const char *wal_path) {
  if (wal_path == NULL) {
    return NULL;
  }

  database_t *db = malloc(sizeof(*db));

  if (db == NULL) {
    return NULL;
  }

  db->map = hashmap_create(0);

  if (db->map == NULL) {
    free(db);
    return NULL;
  }

  db->wal = wal_open(wal_path);

  if (db->wal == NULL) {
    hashmap_destroy(db->map);
    free(db);
    return NULL;
  }

  if (!wal_replay(db->wal, database_replay_callback, db)) {
    wal_close(db->wal);
    hashmap_destroy(db->map);
    free(db);
    return NULL;
  }

  return db;
}

void database_destroy(database_t *db) {
  if (db == NULL) {
    return;
  }

  wal_close(db->wal);
  hashmap_destroy(db->map);
  free(db);
}

bool database_set(database_t *db, const char *key, const char *value) {
  if (db == NULL || key == NULL || value == NULL) {
    return false;
  }

  size_t key_length = strlen(key);
  size_t value_length = strlen(value);

  /*
   * Write to the WAL first. wal_set() makes the record durable
   * before returning.
   */
  if (!wal_set(db->wal, key, key_length, value, value_length)) {
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

  /*
   * Preserve the database API semantics:
   * 	return false when the key doesn't exist
   */
  if (!hashmap_contains(db->map, key)) {
    return false;
  }

  /*
   * Make the deletion durable before modifying memory.
   */
  if (!wal_delete(db->wal, key, strlen(key))) {
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
