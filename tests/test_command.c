#include <kvstore/command.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *TEST_WAL_PATH = "test_command.wal";

static void cleanup_wal(void) { unlink(TEST_WAL_PATH); }

static database_t *create_test_database(void) {
  database_t *db;

  cleanup_wal();

  db = database_open(TEST_WAL_PATH);
  assert(db != NULL);

  return db;
}

static protocol_response_t execute_command(database_t *db,
                                           protocol_message_type_t type,
                                           const char *key, const char *value) {
  protocol_request_t request;
  protocol_response_t response;

  memset(&request, 0, sizeof(request));
  memset(&response, 0, sizeof(response));

  request.type = type;

  if (key != NULL) {
    request.key = (const uint8_t *)key;
    request.key_length = (uint32_t)strlen(key);
  }

  if (value != NULL) {
    request.value = (const uint8_t *)value;
    request.value_length = (uint32_t)strlen(value);
  }

  assert(command_execute(db, &request, &response) == 0);

  return response;
}

static void test_set(void) {
  database_t *db = create_test_database();

  protocol_response_t response =
      execute_command(db, PROTOCOL_MSG_SET, "name", "kvstore");

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(response.data == NULL);
  assert(response.data_length == 0);

  assert(database_size(db) == 1);
  assert(database_contains(db, "name"));
  assert(strcmp(database_get(db, "name"), "kvstore") == 0);

  database_destroy(db);
  cleanup_wal();
}

static void test_get(void) {
  database_t *db = create_test_database();

  protocol_response_t set_response =
      execute_command(db, PROTOCOL_MSG_SET, "name", "kvstore");

  assert(set_response.status == PROTOCOL_STATUS_OK);

  protocol_response_t response =
      execute_command(db, PROTOCOL_MSG_GET, "name", NULL);

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(response.data != NULL);
  assert(response.data_length == strlen("kvstore"));

  assert(memcmp(response.data, "kvstore", response.data_length) == 0);

  database_destroy(db);
  cleanup_wal();
}

static void test_get_missing_key(void) {
  database_t *db = create_test_database();

  protocol_response_t response =
      execute_command(db, PROTOCOL_MSG_GET, "missing", NULL);

  assert(response.status == PROTOCOL_STATUS_NOT_FOUND);
  assert(response.data == NULL);
  assert(response.data_length == 0);

  database_destroy(db);
  cleanup_wal();
}

static void test_set_update(void) {
  database_t *db = create_test_database();

  protocol_response_t response =
      execute_command(db, PROTOCOL_MSG_SET, "key", "first");

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(database_size(db) == 1);
  assert(strcmp(database_get(db, "key"), "first") == 0);

  response = execute_command(db, PROTOCOL_MSG_SET, "key", "second");

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(database_size(db) == 1);
  assert(strcmp(database_get(db, "key"), "second") == 0);

  response = execute_command(db, PROTOCOL_MSG_GET, "key", NULL);

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(response.data_length == strlen("second"));
  assert(memcmp(response.data, "second", response.data_length) == 0);

  database_destroy(db);
  cleanup_wal();
}

static void test_delete(void) {
  database_t *db = create_test_database();

  protocol_response_t response =
      execute_command(db, PROTOCOL_MSG_SET, "key", "value");

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(database_size(db) == 1);

  response = execute_command(db, PROTOCOL_MSG_DEL, "key", NULL);

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(response.data == NULL);
  assert(response.data_length == 0);

  assert(database_size(db) == 0);
  assert(!database_contains(db, "key"));
  assert(database_get(db, "key") == NULL);

  database_destroy(db);
  cleanup_wal();
}

static void test_delete_missing_key(void) {
  database_t *db = create_test_database();

  protocol_response_t response =
      execute_command(db, PROTOCOL_MSG_DEL, "missing", NULL);

  assert(response.status == PROTOCOL_STATUS_NOT_FOUND);
  assert(response.data == NULL);
  assert(response.data_length == 0);

  assert(database_size(db) == 0);

  database_destroy(db);
  cleanup_wal();
}

static void test_multiple_keys(void) {
  database_t *db = create_test_database();

  protocol_response_t response =
      execute_command(db, PROTOCOL_MSG_SET, "key1", "value1");

  assert(response.status == PROTOCOL_STATUS_OK);

  response = execute_command(db, PROTOCOL_MSG_SET, "key2", "value2");

  assert(response.status == PROTOCOL_STATUS_OK);

  response = execute_command(db, PROTOCOL_MSG_SET, "key3", "value3");

  assert(response.status == PROTOCOL_STATUS_OK);

  assert(database_size(db) == 3);

  response = execute_command(db, PROTOCOL_MSG_GET, "key1", NULL);

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(response.data_length == strlen("value1"));
  assert(memcmp(response.data, "value1", response.data_length) == 0);

  response = execute_command(db, PROTOCOL_MSG_GET, "key2", NULL);

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(response.data_length == strlen("value2"));
  assert(memcmp(response.data, "value2", response.data_length) == 0);

  response = execute_command(db, PROTOCOL_MSG_GET, "key3", NULL);

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(response.data_length == strlen("value3"));
  assert(memcmp(response.data, "value3", response.data_length) == 0);

  database_destroy(db);
  cleanup_wal();
}

static void test_invalid_command(void) {
  database_t *db = create_test_database();

  protocol_request_t request;
  protocol_response_t response;

  memset(&request, 0, sizeof(request));
  memset(&response, 0, sizeof(response));

  request.type = (protocol_message_type_t)999;

  assert(command_execute(db, &request, &response) == -1);
  assert(response.status == PROTOCOL_STATUS_INVALID);
  assert(response.data == NULL);
  assert(response.data_length == 0);

  assert(database_size(db) == 0);

  database_destroy(db);
  cleanup_wal();
}

static void test_null_arguments(void) {
  database_t *db = create_test_database();

  protocol_request_t request;
  protocol_response_t response;

  memset(&request, 0, sizeof(request));
  memset(&response, 0, sizeof(response));

  request.type = PROTOCOL_MSG_GET;
  request.key = (const uint8_t *)"key";
  request.key_length = 3;

  assert(command_execute(NULL, &request, &response) == -1);

  assert(command_execute(db, NULL, &response) == -1);

  assert(command_execute(db, &request, NULL) == -1);

  database_destroy(db);
  cleanup_wal();
}

static void test_empty_value(void) {
  database_t *db = create_test_database();

  protocol_response_t response =
      execute_command(db, PROTOCOL_MSG_SET, "empty", "");

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(database_size(db) == 1);

  response = execute_command(db, PROTOCOL_MSG_GET, "empty", NULL);

  assert(response.status == PROTOCOL_STATUS_OK);
  assert(response.data != NULL);
  assert(response.data_length == 0);

  database_destroy(db);
  cleanup_wal();
}

int main(void) {
  test_set();
  test_get();
  test_get_missing_key();
  test_set_update();
  test_delete();
  test_delete_missing_key();
  test_multiple_keys();
  test_invalid_command();
  test_null_arguments();
  test_empty_value();

  return EXIT_SUCCESS;
}
