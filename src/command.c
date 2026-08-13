#include <kvstore/command.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void command_response_reset(protocol_response_t *response) {
  response->status = PROTOCOL_STATUS_ERROR;
  response->data = NULL;
  response->data_length = 0;
}

static char *copy_bytes_as_string(const uint8_t *data, uint32_t length) {
  char *string;

  string = malloc((size_t)length + 1);
  if (string == NULL) {
    return NULL;
  }

  if (length > 0) {
    memcpy(string, data, length);
  }
  string[length] = '\0';

  return string;
}

static int command_set(database_t *db, const protocol_request_t *request,
                       protocol_response_t *response) {
  char *key;
  char *value;
  bool success;

  key = copy_bytes_as_string(request->key, request->key_length);
  if (key == NULL) {
    return -1;
  }

  value = copy_bytes_as_string(request->value, request->value_length);
  if (value == NULL) {
    free(key);
    return -1;
  }

  success = database_set(db, key, value);

  fprintf(stderr, "database_set(\"%s\", \"%s\") = %s\n", key, value,
          success ? "true" : "false");

  free(value);
  free(key);

  if (!success) {
    response->status = PROTOCOL_STATUS_ERROR;
    return -1;
  }

  response->status = PROTOCOL_STATUS_OK;

  return 0;
}

static int command_get(database_t *db, const protocol_request_t *request,
                       protocol_response_t *response) {
  char *key;
  const char *value;
  size_t value_length;

  key = copy_bytes_as_string(request->key, request->key_length);
  if (key == NULL) {
    return -1;
  }

  value = database_get(db, key);

  free(key);

  if (value == NULL) {
    response->status = PROTOCOL_STATUS_NOT_FOUND;
    response->data = NULL;
    response->data_length = 0;
    return 0;
  }

  value_length = strlen(value);

  response->status = PROTOCOL_STATUS_OK;
  response->data = (const uint8_t *)value;
  response->data_length = (uint32_t)value_length;

  return 0;
}

static int command_delete(database_t *db, const protocol_request_t *request,
                          protocol_response_t *response) {
  char *key;
  bool existed;

  key = copy_bytes_as_string(request->key, request->key_length);
  if (key == NULL) {
    return -1;
  }

  existed = database_contains(db, key);

  if (!existed) {
    free(key);

    response->status = PROTOCOL_STATUS_NOT_FOUND;
    response->data = NULL;
    response->data_length = 0;

    return 0;
  }

  if (!database_delete(db, key)) {
    free(key);
    response->status = PROTOCOL_STATUS_ERROR;
    return -1;
  }

  free(key);

  response->status = PROTOCOL_STATUS_OK;
  response->data = NULL;
  response->data_length = 0;

  return 0;
}

// ------------------------------------------------------------------------------

int command_execute(database_t *db, const protocol_request_t *request,
                    protocol_response_t *response) {
  if (db == NULL || request == NULL || response == NULL) {
    return -1;
  }

  command_response_reset(response);

  switch (request->type) {
  case PROTOCOL_MSG_SET:
    return command_set(db, request, response);

  case PROTOCOL_MSG_GET:
    return command_get(db, request, response);

  case PROTOCOL_MSG_DEL:
    return command_delete(db, request, response);

  default:
    response->status = PROTOCOL_STATUS_INVALID;
    return -1;
  }
}
