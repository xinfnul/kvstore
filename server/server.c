#include <kvstore/byteorder.h>
#include <kvstore/command.h>
#include <kvstore/database.h>
#include <kvstore/network.h>
#include <kvstore/protocol.h>
#include <kvstore/server.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define SERVER_REQUEST_PREFIX_SIZE 6u /* version + type + key_length */
#define SERVER_REQUEST_SUFFIX_SIZE                                             \
  4u /* value length				 */

struct server {
  network_server_t *net;
  database_t *db;
};

/*
 * Outcome of trying to read one complete request off the wire.
 */
typedef enum {
  REQUEST_READ_OK,
  REQUEST_READ_CLOSED,
  REQUEST_READ_ERROR,
} request_read_status_t;

/*
 * network_receive() may return fewer bytes than requested.
 * This helper repeatedly calls network_receive() until exactly `length` bytes
 * have been collected.
 *
 * Returns:
 * 	(ssize_t)length - the full amount was read.
 * 	0 				- the peer closed connection before any
 * 					  byte of *this* call was received.
 * -1				- a network error occured, or peer closed
 * 					  connection in the middle of data being
 * read
 */
static ssize_t read_full(network_connection_t *connection, uint8_t *buffer,
                         size_t length) {
  size_t total_read = 0;

  while (total_read < length) {
    size_t received =
        network_receive(connection, buffer + total_read, length - total_read);

    if (received == SIZE_MAX) {
      return -1;
    }

    if (received == 0) {
      return total_read == 0 ? 0 : -1;
    }

    total_read += received;
  }

  return (ssize_t)total_read;
}

/*
 * Reads one complete, framed request from the connection into a newly
 * allocated buffer suitable for protocol_decode_request().
 *
 * On REQUEST_READ_OK, *out_buffer is malloc'd and owned by the caller
 * and *out_length is its size.
 */
static request_read_status_t read_request(network_connection_t *connection,
                                          uint8_t **out_buffer,
                                          size_t *out_length) {
  uint8_t prefix[SERVER_REQUEST_PREFIX_SIZE];
  uint32_t key_length;
  uint32_t value_length;
  size_t remaining_after_prefix;
  size_t total_size;
  uint8_t *buffer;
  uint8_t *resized;
  ssize_t result;

  result = read_full(connection, prefix, sizeof(prefix));

  if (result == 0) {
    return REQUEST_READ_CLOSED;
  }

  if (result < 0) {
    return REQUEST_READ_ERROR;
  }

  key_length = decode_u32(prefix + 2);

  if (key_length > PROTOCOL_MAX_KEY_SIZE) {
    return REQUEST_READ_ERROR;
  }

  remaining_after_prefix = (size_t)key_length + SERVER_REQUEST_SUFFIX_SIZE;

  buffer = malloc(SERVER_REQUEST_PREFIX_SIZE + remaining_after_prefix);

  if (buffer == NULL) {
    return REQUEST_READ_ERROR;
  }

  memcpy(buffer, prefix, SERVER_REQUEST_PREFIX_SIZE);

  result = read_full(connection, buffer + SERVER_REQUEST_PREFIX_SIZE,
                     remaining_after_prefix);

  if (result <= 0) {
    free(buffer);
    return REQUEST_READ_ERROR;
  }

  value_length = decode_u32(buffer + SERVER_REQUEST_PREFIX_SIZE + key_length);

  if (value_length > PROTOCOL_MAX_VALUE_SIZE) {
    free(buffer);
    return REQUEST_READ_ERROR;
  }

  total_size = SERVER_REQUEST_PREFIX_SIZE + (size_t)key_length +
               SERVER_REQUEST_SUFFIX_SIZE + (size_t)value_length;

  resized = realloc(buffer, total_size);

  if (resized == NULL) {
    free(buffer);
    return REQUEST_READ_ERROR;
  }

  buffer = resized;

  if (value_length > 0) {
    result = read_full(connection, buffer + (total_size - value_length),
                       value_length);

    if (result <= 0) {
      free(buffer);
      return REQUEST_READ_ERROR;
    }
  }

  *out_buffer = buffer;
  *out_length = total_size;

  return REQUEST_READ_OK;
}

/*
 * Sends a fully encoded response buffer, freeing it afterwards.
 *
 * Returns true on success, false on network failure.
 */
static bool send_response(network_connection_t *connection,
                          const protocol_response_t *response) {
  uint8_t *buffer = NULL;
  size_t length = 0;
  bool sent;

  if (protocol_encode_response(response, &buffer, &length) != 0) {
    fprintf(stderr, "server: failure to encode response\n");
    return false;
  }

  sent = network_send(connection, buffer, length);

  if (!sent) {
    fprintf(stderr, "server: failed to send response: %s\n",
            network_error_string(network_last_error()));
  }

  free(buffer);

  return sent;
}

/*
 * Server requests on a single connection, one at a time ( blocking ),
 * until the client disconnects or a network/protocol error occurs.
 */
static void handle_connection(database_t *db,
                              network_connection_t *connection) {
  for (;;) {
    uint8_t *request_buffer = NULL;
    size_t request_length = 0;
    request_read_status_t read_status;
    protocol_request_t request;
    protocol_response_t response;

    read_status = read_request(connection, &request_buffer, &request_length);

    if (read_status == REQUEST_READ_CLOSED) {
      break;
    }

    if (read_status == REQUEST_READ_ERROR) {
      fprintf(stderr, "server: failed to read request: %s\n",
              network_error_string(network_last_error()));
      break;
    }

    if (protocol_decode_request(request_buffer, request_length, &request) !=
        0) {
      /*
       * Bytes were framed correctly but the request itself is malformed.
       */
      response.status = PROTOCOL_STATUS_INVALID;
      response.data = NULL;
      response.data_length = 0;
    } else {
      command_execute(db, &request, &response);
    }

    if (!send_response(connection, &response)) {
      free(request_buffer);
      break;
    }

    free(request_buffer);
  }

  network_connection_destroy(connection);
}

// ------------------------------------------------------------------------------

server_t *server_create(const char *host, uint16_t port, const char *wal_path) {
  database_t *db;
  network_server_t *net;
  server_t *server;

  if (wal_path == NULL) {
    return NULL;
  }

  db = database_open(wal_path);

  if (db == NULL) {
    fprintf(stderr, "server: failed to open database ( wal=%s )\n", wal_path);
    return NULL;
  }

  net = network_server_create(host, port, NETWORK_DEFAULT_BACKLOG);

  if (net == NULL) {
    fprintf(stderr, "server: failed to create listening socket: %s\n",
            network_error_string(network_last_error()));
    database_destroy(db);
    return NULL;
  }

  server = malloc(sizeof(*server));

  if (server == NULL) {
    network_server_destroy(net);
    database_destroy(db);
    return NULL;
  }

  server->net = net;
  server->db = db;

  return server;
}

int server_run(server_t *server) {
  if (server == NULL) {
    return -1;
  }

  for (;;) {
    network_connection_t *connection = network_server_accept(server->net);

    if (connection == NULL) {
      fprintf(stderr, "server: accept failed: %s\n",
              network_error_string(network_last_error()));
      return -1;
    }

    fprintf(stderr, "server: client connected\n");

    /* Blocking single connection at a time, as per current design. */
    handle_connection(server->db, connection);

    fprintf(stderr, "server: client disconnected\n");
  }
}

void server_destroy(server_t *server) {
  if (server == NULL) {
    return;
  }

  network_server_destroy(server->net);
  database_destroy(server->db);

  free(server);
}
