#ifndef KVSTORE_NETWORK_H
#define KVSTORE_NETWORK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Invalid socket descriptor.
 */
#define NETWORK_INVALID_SOCKET (-1)

/*
 * Default TCP backlog used by the server.
 */
#define NETWORK_DEFAULT_BACKLOG 128

/*
 * Maximum address string size required by the network layer.
 */
#define NETWORK_HOST_SIZE 256
#define NETWORK_SERVICE_SIZE 32

/*
 * Opaque network connection.
 */
typedef struct network_connection network_connection_t;

/*
 * Server/listening sockets.
 */
typedef struct network_server network_server_t;

/*
 * Network error codes.
 */
typedef enum {
  NETWORK_OK = 0,
  NETWORK_ERROR_INVALID_ARGUMENT,
  NETWORK_ERROR_SOCKET,
  NETWORK_ERROR_BIND,
  NETWORK_ERROR_LISTEN,
  NETWORK_ERROR_ACCEPT,
  NETWORK_ERROR_CONNECT,
  NETWORK_ERROR_SEND,
  NETWORK_ERROR_RECEIVE,
  NETWORK_ERROR_CLOSED,
  NETWORK_ERROR_SYSTEM,
} network_error_t;

/*
 * Creates a TCP listening socket.
 *
 * host:
 * 	Local address to bind to.
 * 	NULL means all local interfaces.
 *
 * port:
 * 	TCP port in host byte order.
 *
 * backlog:
 * 	Maximum pending connection queue size.
 *
 * Returns:
 * 	Allocated network server on success.
 * 	NULL on failure.
 */
network_server_t *network_server_create(const char *host, uint16_t port,
                                        int backlog);

/*
 * Accept one incoming connection.
 *
 * This call blocks until a client connects.
 *
 * Returns:
 * 	New connection on success.
 * 	NULL on failure.
 */
network_connection_t *network_server_accept(network_server_t *server);

/*
 * Close and destroy the listening server.
 */
void network_server_destroy(network_server_t *server);

/*
 * Connect to a remote TCP server.
 *
 * host:
 * 	Remote hostname or IPv4/IPv6 address.
 *
 * port:
 * 	TCP port in host byte order.
 *
 * Returns:
 * 	Allocated connection on success.
 * 	NULL on failure.
 */
network_connection_t *network_connect(const char *host, uint16_t port);

/*
 * Send exactly length bytes.
 *
 * The function handles partial writes internally.
 *
 * Returns:
 * 	true if all bytes were sent.
 * 	false on error or connection closure.
 */
bool network_send(network_connection_t *connection, const void *data,
                  size_t length);

/*
 * Receive up to length bytes.
 *
 * This is a single receive operation and may return fewer bytes
 * than requested.
 *
 * Returns:
 * 	Number of bytes received.
 * 	0 if the peer closed the connection.
 * 	SIZE_MAX on error.
 */
size_t network_receive(network_connection_t *connection, void *buffer,
                       size_t length);

/*
 * Close and destroy a connection.
 */
void network_connection_destroy(network_connection_t *connection);

/*
 * Return the last network error.
 */
network_error_t network_last_error(void);

/*
 * Convert an error code to a human-readable string.
 */
const char *network_error_string(network_error_t error);

#endif
