#include <kvstore/byteorder.h>
#include <kvstore/protocol.h>

#include <stdlib.h>
#include <string.h>

/*
 * Wire format:
 *
 * 		( Request )
 * 				version			1 byte
 * 				type			1 byte
 * 				key_length		4 bytes
 * 				key				...
 * 				value_length	4 bytes
 * 				value 			...
 *
 *
 * 		( Response )
 * 				version 		1 byte
 * 				status 			1 byte
 * 				data_length		4 bytes
 * 				data 			...
 *
 * All integer fields are stored in big-endian order.
 */

#define REQUEST_FIXED_PREFIX_SIZE 6u
#define REQUEST_FIXED_SUFFIX_SIZE 4u

#define RESPONSE_HEADER_SIZE 6u

static int valid_message_type(protocol_message_type_t type) {
  return type == PROTOCOL_MSG_SET || type == PROTOCOL_MSG_GET ||
         type == PROTOCOL_MSG_DEL;
}

static int valid_status(protocol_status_t status) {
  return status == PROTOCOL_STATUS_OK || status == PROTOCOL_STATUS_NOT_FOUND ||
         status == PROTOCOL_STATUS_INVALID || status == PROTOCOL_STATUS_ERROR;
}

// ------------------------------------------------------------------------------

int protocol_encode_request(const protocol_request_t *request, uint8_t **buffer,
                            size_t *buffer_len) {
  size_t total_size;
  uint8_t *out;

  if (request == NULL || buffer == NULL || buffer_len == NULL) {
    return -1;
  }

  if (!valid_message_type(request->type)) {
    return -1;
  }

  if (request->key_length > PROTOCOL_MAX_KEY_SIZE) {
    return -1;
  }

  if (request->value_length > PROTOCOL_MAX_VALUE_SIZE) {
    return -1;
  }

  /*
   * GET and DEL must not contain a value.
   */
  if ((request->type == PROTOCOL_MSG_GET ||
       request->type == PROTOCOL_MSG_DEL) &&
      request->value_length != 0) {
    return -1;
  }

  if (request->key_length > 0 && request->key == NULL) {
    return -1;
  }

  if (request->value_length > 0 && request->value == NULL) {
    return -1;
  }

  total_size = REQUEST_FIXED_PREFIX_SIZE + (size_t)request->key_length +
               REQUEST_FIXED_SUFFIX_SIZE + (size_t)request->value_length;

  out = malloc(total_size);

  if (out == NULL) {
    return -1;
  }

  out[0] = PROTOCOL_VERSION;
  out[1] = (uint8_t)request->type;

  encode_u32(out + 2, request->key_length);

  if (request->key_length > 0) {
    memcpy(out + REQUEST_FIXED_PREFIX_SIZE, request->key, request->key_length);
  }

  encode_u32(out + REQUEST_FIXED_PREFIX_SIZE + request->key_length,
             request->value_length);

  if (request->value_length > 0) {
    memcpy(out + REQUEST_FIXED_PREFIX_SIZE + request->key_length +
               REQUEST_FIXED_SUFFIX_SIZE,
           request->value, request->value_length);
  }

  *buffer = out;
  *buffer_len = total_size;

  return 0;
}

int protocol_decode_request(const uint8_t *buffer, size_t buffer_len,
                            protocol_request_t *request) {
  uint32_t key_length;
  uint32_t value_length;
  size_t expected_size;

  if (buffer == NULL || request == NULL) {
    return -1;
  }

  if (buffer_len < REQUEST_FIXED_PREFIX_SIZE + REQUEST_FIXED_SUFFIX_SIZE) {
    return -1;
  }

  if (buffer[0] != PROTOCOL_VERSION) {
    return -1;
  }

  if (!valid_message_type((protocol_message_type_t)buffer[1])) {
    return -1;
  }

  key_length = decode_u32(buffer + 2);

  if (key_length > PROTOCOL_MAX_KEY_SIZE) {
    return -1;
  }

  /*
   * Fixed header is 10 bytes.
   * version + type + key_length + value_length
   * The key occupies the bytes between the key_length and value_length.
   */
  if ((size_t)key_length >
      buffer_len - (REQUEST_FIXED_PREFIX_SIZE + REQUEST_FIXED_SUFFIX_SIZE)) {
    return -1;
  }

  value_length = decode_u32(buffer + REQUEST_FIXED_PREFIX_SIZE + key_length);

  if (value_length > PROTOCOL_MAX_VALUE_SIZE) {
    return -1;
  }

  expected_size = REQUEST_FIXED_PREFIX_SIZE + (size_t)key_length +
                  REQUEST_FIXED_SUFFIX_SIZE + (size_t)value_length;

  if (expected_size != buffer_len) {
    return -1;
  }

  if ((buffer[1] == PROTOCOL_MSG_GET || buffer[1] == PROTOCOL_MSG_DEL) &&
      value_length != 0) {
    return -1;
  }

  request->type = (protocol_message_type_t)buffer[1];
  request->key = buffer + REQUEST_FIXED_PREFIX_SIZE;
  request->key_length = key_length;

  request->value = buffer + REQUEST_FIXED_PREFIX_SIZE + key_length +
                   REQUEST_FIXED_SUFFIX_SIZE;

  request->value_length = value_length;

  return 0;
}

int protocol_encode_response(const protocol_response_t *response,
                             uint8_t **buffer, size_t *buffer_len) {
  size_t total_size;
  uint8_t *out;

  if (response == NULL || buffer == NULL || buffer_len == NULL) {
    return -1;
  }

  if (!valid_status(response->status)) {
    return -1;
  }

  if (response->data_length > PROTOCOL_MAX_VALUE_SIZE) {
    return -1;
  }

  if (response->data_length > 0 && response->data == NULL) {
    return -1;
  }

  total_size = RESPONSE_HEADER_SIZE + (size_t)response->data_length;

  out = malloc(total_size);

  if (out == NULL) {
    return -1;
  }

  out[0] = PROTOCOL_VERSION;
  out[1] = (uint8_t)response->status;

  encode_u32(out + 2, response->data_length);

  if (response->data_length > 0) {
    memcpy(out + RESPONSE_HEADER_SIZE, response->data, response->data_length);
  }

  *buffer = out;
  *buffer_len = total_size;

  return 0;
}

int protocol_decode_response(const uint8_t *buffer, size_t buffer_len,
                             protocol_response_t *response) {
  uint32_t data_length;
  size_t expected_size;

  if (buffer == NULL || response == NULL) {
    return -1;
  }

  if (buffer_len < RESPONSE_HEADER_SIZE) {
    return -1;
  }

  if (buffer[0] != PROTOCOL_VERSION) {
    return -1;
  }

  if (!valid_status((protocol_status_t)buffer[1])) {
    return -1;
  }

  data_length = decode_u32(buffer + 2);

  if (data_length > PROTOCOL_MAX_VALUE_SIZE) {
    return -1;
  }

  expected_size = RESPONSE_HEADER_SIZE + (size_t)data_length;

  if (expected_size != buffer_len) {
    return -1;
  }

  response->status = (protocol_status_t)buffer[1];
  response->data = buffer + RESPONSE_HEADER_SIZE;
  response->data_length = data_length;

  return 0;
}
