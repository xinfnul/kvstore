#include <kvstore/protocol.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_set_request(void) {
  const uint8_t key[] = "name";
  const uint8_t value[] = "kvstore";

  protocol_request_t request = {
      .type = PROTOCOL_MSG_SET,
      .key = key,
      .key_length = sizeof(key) - 1,
      .value = value,
      .value_length = sizeof(value) - 1,
  };

  uint8_t *buffer = NULL;
  size_t buffer_len = 0;

  assert(protocol_encode_request(&request, &buffer, &buffer_len) == 0);

  assert(buffer != NULL);
  assert(buffer_len > 0);

  protocol_request_t decoded;

  assert(protocol_decode_request(buffer, buffer_len, &decoded) == 0);

  assert(decoded.type == PROTOCOL_MSG_SET);
  assert(decoded.key_length == 4);
  assert(decoded.value_length == 7);

  assert(memcmp(decoded.key, "name", 4) == 0);
  assert(memcmp(decoded.value, "kvstore", 7) == 0);

  free(buffer);
}

static void test_get_request(void) {
  const uint8_t key[] = "name";

  protocol_request_t request = {
      .type = PROTOCOL_MSG_GET,
      .key = key,
      .key_length = sizeof(key) - 1,
      .value = NULL,
      .value_length = 0,
  };

  uint8_t *buffer = NULL;
  size_t buffer_len = 0;

  assert(protocol_encode_request(&request, &buffer, &buffer_len) == 0);

  assert(buffer != NULL);
  assert(buffer_len > 0);

  protocol_request_t decoded;

  assert(protocol_decode_request(buffer, buffer_len, &decoded) == 0);

  assert(decoded.type == PROTOCOL_MSG_GET);
  assert(decoded.key_length == 4);
  assert(decoded.value_length == 0);

  assert(memcmp(decoded.key, "name", 4) == 0);

  free(buffer);
}

static void test_del_request(void) {
  const uint8_t key[] = "name";

  protocol_request_t request = {
      .type = PROTOCOL_MSG_DEL,
      .key = key,
      .key_length = sizeof(key) - 1,
      .value = NULL,
      .value_length = 0,
  };

  uint8_t *buffer = NULL;
  size_t buffer_len = 0;

  assert(protocol_encode_request(&request, &buffer, &buffer_len) == 0);

  assert(buffer != NULL);
  assert(buffer_len > 0);

  protocol_request_t decoded;

  assert(protocol_decode_request(buffer, buffer_len, &decoded) == 0);

  assert(decoded.type == PROTOCOL_MSG_DEL);
  assert(decoded.key_length == 4);
  assert(decoded.value_length == 0);

  assert(memcmp(decoded.key, "name", 4) == 0);

  free(buffer);
}

static void test_response_ok(void) {
  const uint8_t data[] = "kvstore";

  protocol_response_t response = {
      .status = PROTOCOL_STATUS_OK,
      .data = data,
      .data_length = sizeof(data) - 1,
  };

  uint8_t *buffer = NULL;
  size_t buffer_len = 0;

  assert(protocol_encode_response(&response, &buffer, &buffer_len) == 0);

  protocol_response_t decoded;

  assert(protocol_decode_response(buffer, buffer_len, &decoded) == 0);

  assert(decoded.status == PROTOCOL_STATUS_OK);
  assert(decoded.data_length == 7);
  assert(memcmp(decoded.data, "kvstore", 7) == 0);

  free(buffer);
}

static void test_response_not_found(void) {
  protocol_response_t response = {
      .status = PROTOCOL_STATUS_NOT_FOUND, .data = NULL, .data_length = 0};

  uint8_t *buffer = NULL;
  size_t buffer_len = 0;

  assert(protocol_encode_response(&response, &buffer, &buffer_len) == 0);

  protocol_response_t decoded;

  assert(protocol_decode_response(buffer, buffer_len, &decoded) == 0);

  assert(decoded.status == PROTOCOL_STATUS_NOT_FOUND);
  assert(decoded.data_length == 0);

  free(buffer);
}

static void test_invalid_protocol_version(void) {
  const uint8_t key[] = "name";

  protocol_request_t request = {.type = PROTOCOL_MSG_GET,
                                .key = key,
                                .key_length = sizeof(key) - 1,
                                .value = NULL,
                                .value_length = 0};

  uint8_t *buffer = NULL;
  size_t buffer_len = 0;

  assert(protocol_encode_request(&request, &buffer, &buffer_len) == 0);

  buffer[0] = 99;

  protocol_request_t decoded;

  assert(protocol_decode_request(buffer, buffer_len, &decoded) == -1);

  free(buffer);
}

static void test_truncated_request(void) {
  const uint8_t key[] = "name";

  protocol_request_t request = {
      .type = PROTOCOL_MSG_GET,
      .key = key,
      .key_length = sizeof(key) - 1,
      .value = NULL,
      .value_length = 0,
  };

  uint8_t *buffer = NULL;
  size_t buffer_len = 0;

  assert(protocol_encode_request(&request, &buffer, &buffer_len) == 0);

  assert(buffer != NULL);
  assert(buffer_len > 0);

  protocol_request_t decoded;

  assert(protocol_decode_request(buffer, buffer_len - 1, &decoded) == -1);

  free(buffer);
}

static void test_trailing_bytes(void) {
  const uint8_t key[] = "name";

  protocol_request_t request = {.type = PROTOCOL_MSG_GET,
                                .key = key,
                                .key_length = sizeof(key) - 1,
                                .value = NULL,
                                .value_length = 0};

  uint8_t *buffer = NULL;
  size_t buffer_len = 0;

  assert(protocol_encode_request(&request, &buffer, &buffer_len) == 0);

  uint8_t *invalid_buffer = malloc(buffer_len + 1);
  assert(invalid_buffer != NULL);

  memcpy(invalid_buffer, buffer, buffer_len);
  invalid_buffer[buffer_len] = 0xff;

  protocol_request_t decoded;

  assert(protocol_decode_request(invalid_buffer, buffer_len + 1, &decoded) ==
         -1);

  free(invalid_buffer);
  free(buffer);
}

static void test_invalid_message_type(void) {
  uint8_t buffer[] = {
      PROTOCOL_VERSION, 99, 0, 0, 0, 4, 'n', 'a', 'm', 'e', 0, 0, 0, 0};

  protocol_request_t decoded;

  assert(protocol_decode_request(buffer, sizeof(buffer), &decoded) == -1);
}

static void test_get_with_value_rejected(void) {
  uint8_t buffer[] = {PROTOCOL_VERSION, PROTOCOL_MSG_GET,

                      /* key length = 4 */
                      0, 0, 0, 4,

                      'n', 'a', 'm', 'e',

                      /* value length = 1 */
                      0, 0, 0, 1,

                      'x'};

  protocol_request_t decoded;

  assert(protocol_decode_request(buffer, sizeof(buffer), &decoded) == -1);
}

static void test_null_arguments(void) {
  protocol_request_t request = {0};
  protocol_response_t response = {0};

  uint8_t *buffer = NULL;
  size_t buffer_len = 0;

  assert(protocol_encode_request(NULL, &buffer, &buffer_len) == -1);

  assert(protocol_encode_request(&request, NULL, &buffer_len) == -1);

  assert(protocol_encode_request(&request, &buffer, NULL) == -1);

  assert(protocol_decode_request(NULL, 0, &request) == -1);

  assert(protocol_encode_response(NULL, &buffer, &buffer_len) == -1);

  assert(protocol_encode_response(&response, NULL, &buffer_len) == -1);

  assert(protocol_decode_response(NULL, 0, &response) == -1);
}

int main(void) {
  test_set_request();
  test_get_request();
  test_del_request();

  test_response_ok();
  test_response_not_found();

  test_invalid_protocol_version();
  test_truncated_request();
  test_trailing_bytes();
  test_invalid_message_type();
  test_get_with_value_rejected();
  test_null_arguments();

  return EXIT_SUCCESS;
}
