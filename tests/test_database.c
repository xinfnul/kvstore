#include "database.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_WAL_PATH "test_database.wal"

static void remove_test_wal(void) { (void)unlink(TEST_WAL_PATH); }

static void test_open_empty_database(void) {
  remove_test_wal();

  database_t *db = database_open(TEST_WAL_PATH);

  assert(db != NULL);
  assert(database_size(db) == 0);
  assert(!database_contains(db, "random"));
  assert(database_get(db, "random") == NULL);

  database_destroy(db);
  remove_test_wal();
}

static void test_set_get(void) {
  remove_test_wal();

  database_t *db = database_open(TEST_WAL_PATH);
  assert(db != NULL);

  assert(database_set(db, "name", "kvstore"));
  assert(database_size(db) == 1);
  assert(database_contains(db, "name"));

  const char *value = database_get(db, "name");

  assert(value != NULL);
  assert(strcmp(value, "kvstore") == 0);

  remove_test_wal();
}

static void test_update(void) {
  remove_test_wal();

  database_t *db = database_open(TEST_WAL_PATH);
  assert(db != NULL);

  assert(database_set(db, "name", "kvs"));
  assert(database_set(db, "name", "kvstore"));

  assert(database_size(db) == 1);

  const char *value = database_get(db, "name");

  assert(value != NULL);
  assert(strcmp(value, "kvstore") == 0);

  database_destroy(db);
  remove_test_wal();
}

static void test_delete(void) {
  remove_test_wal();

  database_t *db = database_open(TEST_WAL_PATH);
  assert(db != NULL);

  assert(database_set(db, "name", "kvstore"));
  assert(database_size(db) == 1);

  assert(database_delete(db, "name"));
  assert(database_size(db) == 0);
  assert(!database_contains(db, "name"));
  assert(database_get(db, "name") == NULL);

  assert(!database_delete(db, "name"));

  database_destroy(db);
  remove_test_wal();
}

static void test_multiple_keys(void) {
  remove_test_wal();

  database_t *db = database_open(TEST_WAL_PATH);
  assert(db != NULL);

  assert(database_set(db, "language", "C"));
  assert(database_set(db, "project", "kvstore"));

  assert(database_size(db) == 2);

  assert(strcmp(database_get(db, "language"), "C") == 0);
  assert(strcmp(database_get(db, "project"), "kvstore") == 0);

  database_destroy(db);
  remove_test_wal();
}

static void test_persistence(void) {
  remove_test_wal();

  /*
   * First process.
   */
  database_t *db = database_open(TEST_WAL_PATH);
  assert(db != NULL);

  assert(database_set(db, "name", "kvstore"));
  assert(database_set(db, "language", "C"));

  assert(database_delete(db, "language"));

  database_destroy(db);

  /*
   * Second process.
   */
  db = database_open(TEST_WAL_PATH);
  assert(db != NULL);

  assert(database_size(db) == 1);

  assert(database_contains(db, "name"));
  assert(!database_contains(db, "language"));

  assert(strcmp(database_get(db, "name"), "kvstore") == 0);
  assert(database_get(db, "language") == NULL);

  database_destroy(db);

  remove_test_wal();
}

static void test_invalid_arguments(void) {
  remove_test_wal();

  assert(database_open(NULL) == NULL);

  database_t *db = database_open(TEST_WAL_PATH);
  assert(db != NULL);

  assert(!database_set(NULL, "key", "value"));
  assert(!database_set(db, NULL, "value"));
  assert(!database_set(db, "key", NULL));

  assert(database_get(NULL, "key") == NULL);
  assert(database_get(db, NULL) == NULL);

  assert(!database_delete(NULL, "key"));
  assert(!database_delete(db, NULL));

  assert(!database_contains(NULL, "key"));
  assert(!database_contains(db, NULL));

  assert(database_size(NULL) == 0);

  database_destroy(NULL);
  database_destroy(db);

  remove_test_wal();
}

int main(void) {
  test_open_empty_database();
  test_set_get();
  test_update();
  test_delete();
  test_multiple_keys();
  test_invalid_arguments();

  return EXIT_SUCCESS;
}
