#include <kvstore/client.h>

#include <kvstore/byteorder.h>
#include <kvstore/network.h>
#include <kvstore/protocol.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLIENT_RESPONSE_HEADER_SIZE 6u

struct client {
  network_connection_t *conn;
  char last_error[256];
};

static void set_error(client_t *client, const char *message) {
  if (client == NULL) {
    return;
  }

  snprintf(client->last_error, sizeof(client->last_error), "%s", message);
}

/*
 * network_receive() may return fewer bytes than requested, so this
 * loops until exactly `length` bytes have been read, or fails.
 */
static bool read_full(client_t *client, uint8_t *buffer, size_t length) {
  size_t total_read = 0;

  while (total_read < length) {
    size_t received =
        network_receive(client->conn, buffer + total_read, length - total_read);

    if (received == SIZE_MAX) {
      set_error(client, network_error_string(network_last_error()));
      return false;
    }

    if (received == 0) {
      set_error(client, "connection closed by server");
      return false;
    }

    total_read += received;
  }

  return true;
}

/*
 * Encodes and sends one request.
 */
static int send_request(client_t *client, const protocol_request_t *request) {
  uint8_t *buffer = NULL;
  size_t length = 0;

  if (protocol_encode_request(request, &buffer, &length) != 0) {
    set_error(client, "failed to encode request ( ey/value too large? )");
    return -1;
  }

  bool sent = network_send(client->conn, buffer, length);

  free(buffer);

  if (!sent) {
    set_error(client, network_error_string(network_last_error()));
    return -1;
  }

  return 0;
}

/*
 * Reads one complete response into a newly allocated buffer suitable
 * for protocol_decode_response().
 */
static int read_response(client_t *client, uint8_t **out_buffer,
                         size_t *out_length) {
  uint8_t header[CLIENT_RESPONSE_HEADER_SIZE];

  if (!read_full(client, header, sizeof(header))) {
    return -1;
  }

  uint32_t data_length = decode_u32(header + 2);

  if (data_length > PROTOCOL_MAX_VALUE_SIZE) {
    set_error(client, "malformed response: value too large");
    return -1;
  }

  size_t total_size = CLIENT_RESPONSE_HEADER_SIZE + (size_t)data_length;
  uint8_t *buffer = malloc(total_size);

  if (buffer == NULL) {
    set_error(client, "out of memory");
    return -1;
  }

  memcpy(buffer, header, CLIENT_RESPONSE_HEADER_SIZE);

  if (data_length > 0) {
    if (!read_full(client, buffer + CLIENT_RESPONSE_HEADER_SIZE, data_length)) {
      free(buffer);
      return -1;
    }
  }

  *out_buffer = buffer;
  *out_length = total_size;

  return 0;
}

/*
 * Sends `request`, reads the response, and decoded it into
 * *out_response. *out_response->data ( if any ) points into
 * *out_raw_buffer, which the caller owns and must free() once done
 * reading the response.
 */
static int perform_request(client_t *client, const protocol_request_t *request,
                           protocol_response_t *out_response,
                           uint8_t **out_raw_buffer) {
  if (send_request(client, request) != 0) {
    return -1;
  }

  uint8_t *resp_buffer = NULL;
  size_t resp_length = 0;

  if (read_response(client, &resp_buffer, &resp_length) != 0) {
    return -1;
  }

  if (protocol_decode_response(resp_buffer, resp_length, out_response) != 0) {
    free(resp_buffer);
    set_error(client, "malformed response from server");
    return -1;
  }

  *out_raw_buffer = resp_buffer;

  return 0;
}

// ------------------------------------------------------------------------------

client_t *client_connect(const char *host, uint16_t port) {
  network_connection_t *conn = network_connect(host, port);

  if (conn == NULL) {
    fprintf(stderr, "client: failed to connect to %s:%u: %s\n",
            host != NULL ? host : "( null )", (unsigned)port,
            network_error_string(network_last_error()));
    return NULL;
  }

  client_t *client = malloc(sizeof(*client));

  if (client == NULL) {
    network_connection_destroy(conn);
    fprintf(stderr, "client: out of memory\n");
    return NULL;
  }

  client->conn = conn;
  client->last_error[0] = '\0';

  return client;
}

int client_set(client_t *client, const uint8_t *key, uint32_t key_length,
               const uint8_t *value, uint32_t value_length,
               protocol_status_t *out_status) {
  if (client == NULL || out_status == NULL) {
    return -1;
  }

  protocol_request_t request = {.type = PROTOCOL_MSG_SET,
                                .key = key,
                                .key_length = key_length,
                                .value = value,
                                .value_length = value_length};

  protocol_response_t response;
  uint8_t *raw_buffer = NULL;

  if (perform_request(client, &request, &response, &raw_buffer) != 0) {
    return -1;
  }

  *out_status = response.status;

  free(raw_buffer);

  return 0;
}

int client_get(client_t *client, const uint8_t *key, uint32_t key_length,
               protocol_status_t *out_status, uint8_t **out_value,
               uint32_t *out_value_length) {
  if (client == NULL || out_status == NULL || out_value == NULL ||
      out_value_length == NULL) {
    return -1;
  }

  protocol_request_t request = {
      .type = PROTOCOL_MSG_GET,
      .key = key,
      .key_length = key_length,
      .value = NULL,
      .value_length = 0,
  };

  protocol_response_t response;
  uint8_t *raw_buffer = NULL;

  if (perform_request(client, &request, &response, &raw_buffer) != 0) {
    return -1;
  }

  if (response.status == PROTOCOL_STATUS_OK) {
    /*
     * Always allocate at least 1 byte so *out_value is never NULL on
     * a successful OK response, even for an empty value.
     */
    size_t alloc_size = response.data_length > 0 ? response.data_length : 1;
    uint8_t *value = malloc(alloc_size);

    if (value == NULL) {
      free(raw_buffer);
      set_error(client, "out of memory");
      return -1;
    }

    if (response.data_length > 0) {
      memcpy(value, response.data, response.data_length);
    }

    *out_value = value;
    *out_value_length = response.data_length;
  } else {
    *out_value = NULL;
    *out_value_length = 0;
  }

  *out_status = response.status;

  free(raw_buffer);

  return 0;
}

int client_del(client_t *client, const uint8_t *key, uint32_t key_length,
               protocol_status_t *out_status) {
  if (client == NULL || out_status == NULL) {
    return -1;
  }

  protocol_request_t request = {
      .type = PROTOCOL_MSG_DEL,
      .key = key,
      .key_length = key_length,
      .value = NULL,
      .value_length = 0,
  };

  protocol_response_t response;
  uint8_t *raw_buffer = NULL;

  if (perform_request(client, &request, &response, &raw_buffer) != 0) {
    return -1;
  }

  *out_status = response.status;

  free(raw_buffer);

  return 0;
}

const char *client_last_error_string(const client_t *client) {
  if (client == NULL) {
    return "invalid client handle";
  }

  return client->last_error;
}

void client_disconnect(client_t *client) {
  if (client == NULL) {
    return;
  }

  network_connection_destroy(client->conn);

  free(client);
}
