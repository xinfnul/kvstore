#ifndef KVSTORE_DATABASE_H
#define KVSTORE_DATABASE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct database database_t;

/*
 * Open or create a new database backed by a WAL file.
 *
 * If the WAL already contains records, they are replayed to
 * reconstruct the current database state.
 *
 * Returns NULL on allocation failure, WAL failure, or
 * WAL replay failure.
 */
database_t *database_open(const char *wal_path);

/*
 * Destroy the database and release all resources.
 *
 * The WAL is closed, but its file is not deleted.
 */
void database_destroy(database_t *db);

/*
 * Insert or update key/value pair.
 *
 * The operation is written to the WAL and made durable before
 * the in-memory database is updated.
 *
 * The database makes its own copies of key and value.
 *
 * Returns true on success, false on allocation failure.
 */
bool database_set(database_t *db, const char *key, const char *value);

/*
 * Retrieve the value associated with key.
 *
 * Returns NULL if the key does not exist.
 *
 * The returned pointer is owned by the database and remains valid
 * until the key is updated, deleted, or the database is destroyed.
 */
const char *database_get(const database_t *db, const char *key);

/*
 * Delete a key-value pair.
 *
 * The deletion is written to the WAL and made durable before
 * the in-memory database is updated.
 *
 * Returns true if the key existed and was deleted.
 * Returns false if the key does not exist.
 */
bool database_delete(database_t *db, const char *key);

/*
 * Check whether a key exists.
 */
bool database_contains(const database_t *db, const char *key);

/*
 * Return the number of key-value pairs.
 */
size_t database_size(const database_t *db);

#endif
