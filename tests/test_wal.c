#include "wal.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_WAL_PATH "test_wal.wal"

typedef struct {
  size_t count;
  size_t set_count;
  size_t delete_count;
} replay_state_t;

static bool replay_callback(wal_operation_t operation, const char *key,
                            size_t key_length, const char *value,
                            size_t value_length, void *user_data) {
  replay_state_t *state = user_data;

  assert(state != NULL);
  assert(key != NULL);
  assert(key_length > 0);

  state->count++;

  if (operation == WAL_OP_SET) {
    state->set_count++;

    assert(value != NULL);
    assert(value_length > 0);
  } else if (operation == WAL_OP_DELETE) {
    state->delete_count++;

    assert(value == NULL);
    assert(value_length == 0);
  } else {
    return false;
  }

  return true;
}

static void remove_test_wal(void) { unlink(TEST_WAL_PATH); }

static void test_open_close(void) {
  wal_t *wal = wal_open(TEST_WAL_PATH);

  assert(wal != NULL);

  wal_close(wal);

  remove_test_wal();
}

static void test_set(void) {
  wal_t *wal = wal_open(TEST_WAL_PATH);

  assert(wal != NULL);

  assert(wal_set(wal, "name", strlen("name"), "kvstore", strlen("kvstore")));

  wal_close(wal);

  wal = wal_open(TEST_WAL_PATH);

  assert(wal != NULL);

  replay_state_t state = {0};

  assert(wal_replay(wal, replay_callback, &state));

  assert(state.count == 1);
  assert(state.set_count == 1);
  assert(state.delete_count == 0);

  wal_close(wal);

  remove_test_wal();
}

static void test_delete(void) {
  wal_t *wal = wal_open(TEST_WAL_PATH);

  assert(wal != NULL);

  assert(wal_delete(wal, "name", strlen("name")));

  wal_close(wal);

  wal = wal_open(TEST_WAL_PATH);

  assert(wal != NULL);

  replay_state_t state = {0};

  assert(wal_replay(wal, replay_callback, &state));

  assert(state.count == 1);
  assert(state.set_count == 0);
  assert(state.delete_count == 1);

  wal_close(wal);

  remove_test_wal();
}

static void test_multiple_operations(void) {
  wal_t *wal = wal_open(TEST_WAL_PATH);

  assert(wal != NULL);

  assert(wal_set(wal, "name", strlen("name"), "kvstore", strlen("kvstore")));

  assert(wal_set(wal, "language", strlen("language"), "C", strlen("C")));

  assert(wal_delete(wal, "name", strlen("name")));

  wal_close(wal);

  wal = wal_open(TEST_WAL_PATH);

  assert(wal != NULL);

  replay_state_t state = {0};

  assert(wal_replay(wal, replay_callback, &state));

  assert(state.count == 3);
  assert(state.set_count == 2);
  assert(state.delete_count == 1);

  wal_close(wal);

  remove_test_wal();
}

static void test_invalid_arguments(void) {
  wal_t *wal = wal_open(TEST_WAL_PATH);

  assert(wal != NULL);

  assert(!wal_set(wal, NULL, 4, "value", 5));

  assert(!wal_set(wal, "key", 3, NULL, 0));

  assert(!wal_delete(wal, NULL, 3));

  assert(!wal_delete(wal, NULL, 0));

  assert(!wal_replay(wal, NULL, NULL));

  wal_close(wal);

  remove_test_wal();
}

int main(void) {
  test_open_close();
  test_set();
  test_delete();
  test_multiple_operations();
  test_invalid_arguments();

  return EXIT_SUCCESS;
}
