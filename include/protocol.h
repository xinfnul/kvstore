#ifndef KVSTORE_PROTOCOL_H
#define KVSTORE_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define PROTOCOL_VERSION 1

#define PROTOCOL_MAX_KEY_SIZE (1024u * 1024u)
#define PROTOCOL_MAX_VALUE_SIZE (64u * 1024u * 1024u)

/*
 * Message types.
 */
typedef enum {
  PROTOCOL_MSG_SET = 1,
  PROTOCOL_MSG_GET = 2,
  PROTOCOL_MSG_DEL = 3,
} protocol_message_type_t;

/*
 * Response status.
 */
typedef enum {
  PROTOCOL_STATUS_OK = 0,
  PROTOCOL_STATUS_NOT_FOUND = 1,
  PROTOCOL_STATUS_INVALID = 2,
  PROTOCOL_STATUS_ERROR = 3,
} protocol_status_t;

/*
 * A decoded request.
 *
 * key and value point into caller-owned memory.
 * protocol_request_destroy() does not free them.
 */
typedef struct {
  protocol_message_type_t type;

  const uint8_t *key;
  uint32_t key_length;

  const uint8_t *value;
  uint32_t value_length;
} protocol_request_t;

/*
 * A decoded response.
 *
 * data points into caller-owned memory.
 */
typedef struct {
  protocol_status_t status;

  const uint8_t *data;
  uint32_t data_length;
} protocol_response_t;

/*
 * Encode a request into a newly allocated buffer.
 *
 * The caller owns the returned buffer and must free() it.
 *
 * Returns:
 * 	0 on success
 * 	-1 on invalid input or allocation failure
 */
int protocol_encode_request(const protocol_request_t *request, uint8_t **buffer,
                            size_t *buffer_len);

/*
 * Decode a request from a byte buffer.
 *
 * The decoded request points into the supplied buffer.
 * The buffer must remain valid while the request is being used.
 *
 * Returns:
 * 	0 on success
 * 	-1 on malformed or invalid input
 */
int protocol_decode_request(const uint8_t *buffer, size_t buffer_len,
                            protocol_request_t *request);

/*
 * Encode a response into a newly allocated buffer.
 *
 * The caller owns the returned buffer and must free() it.
 */
int protocol_encode_response(const protocol_response_t *response,
                             uint8_t **buffer, size_t *buffer_len);

/*
 * Decode a response from a byte buffer.
 *
 * THe decoded response points into the supplied buffer.
 */
int protocol_decode_response(const uint8_t *buffer, size_t buffer_len,
                             protocol_response_t *response);

#endif
