#ifndef KVSTORE_WAL_H
#define KVSTORE_WAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct wal wal_t;

typedef enum { WAL_OP_SET = 1, WAL_OP_DELETE = 2 } wal_operation_t;

typedef bool (*wal_replay_callback)(wal_operation_t operation, const char *key,
                                    size_t key_length, const char *value,
                                    size_t value_length, void *user_data);

/*
 * Open or create a WAL file.
 */
wal_t *wal_open(const char *path);

/*
 * Close the WAL and release resources.
 */
void wal_close(wal_t *wal);

/*
 * Append a SET operation and make it durable.
 */
bool wal_set(wal_t *wal, const char *key, size_t key_length, const char *value,
             size_t value_length);

/*
 * Append a DELETE operation and make it durable.
 */
bool wal_delete(wal_t *wal, const char *key, size_t key_length);

/*
 *	Replay all valid records in the WAL.
 *
 * Stops and returns false if a corrupted record is encountered.
 */
bool wal_replay(wal_t *wal, wal_replay_callback callback, void *user_data);

#endif
