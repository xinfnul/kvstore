#define _POSIX_C_SOURCE 200809L

#include <kvstore/network.h>

#include <errno.h>
#include <netdb.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <unistd.h>

struct network_server {
  int fd;
};

struct network_connection {
  int fd;
};

static network_error_t g_last_error = NETWORK_OK;

static void set_error(network_error_t error) { g_last_error = error; }

static int create_tcp_socket(const struct addrinfo *address) {
  return socket(address->ai_family, address->ai_socktype, address->ai_protocol);
}

// ------------------------------------------------------------------------------

network_server_t *network_server_create(const char *host, uint16_t port,
                                        int backlog) {
  if (backlog <= 0) {
    set_error(NETWORK_ERROR_INVALID_ARGUMENT);
    return NULL;
  }

  char port_string[NETWORK_SERVICE_SIZE];

  int written =
      snprintf(port_string, sizeof(port_string), "%u", (unsigned)port);

  if (written < 0 || (size_t)written >= sizeof(port_string)) {
    set_error(NETWORK_ERROR_INVALID_ARGUMENT);
    return NULL;
  }

  struct addrinfo hints;
  struct addrinfo *results = NULL;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int result = getaddrinfo(host, port_string, &hints, &results);

  if (result != 0) {
    set_error(NETWORK_ERROR_BIND);
    return NULL;
  }

  int server_fd = NETWORK_INVALID_SOCKET;

  for (struct addrinfo *address = results; address != NULL;
       address = address->ai_next) {
    int fd = create_tcp_socket(address);

    if (fd == NETWORK_INVALID_SOCKET) {
      continue;
    }

    int reuse = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
      close(fd);
      continue;
    }

    if (bind(fd, address->ai_addr, address->ai_addrlen) == 0) {
      server_fd = fd;
      break;
    }

    close(fd);
  }

  freeaddrinfo(results);

  if (server_fd == NETWORK_INVALID_SOCKET) {
    set_error(NETWORK_ERROR_BIND);
    return NULL;
  }

  if (listen(server_fd, backlog) < 0) {
    close(server_fd);
    set_error(NETWORK_ERROR_LISTEN);
    return NULL;
  }

  network_server_t *server = malloc(sizeof(*server));

  if (server == NULL) {
    close(server_fd);
    set_error(NETWORK_ERROR_SYSTEM);
    return NULL;
  }

  server->fd = server_fd;

  set_error(NETWORK_OK);

  return server;
}

network_connection_t *network_server_accept(network_server_t *server) {
  if (server == NULL) {
    set_error(NETWORK_ERROR_INVALID_ARGUMENT);
    return NULL;
  }

  int client_fd = accept(server->fd, NULL, NULL);

  if (client_fd < 0) {
    set_error(NETWORK_ERROR_ACCEPT);
    return NULL;
  }

  network_connection_t *connection = malloc(sizeof(*connection));

  if (connection == NULL) {
    close(client_fd);
    set_error(NETWORK_ERROR_SYSTEM);
    return NULL;
  }

  connection->fd = client_fd;

  set_error(NETWORK_OK);

  return connection;
}

void network_server_destroy(network_server_t *server) {
  if (server == NULL) {
    return;
  }

  if (server->fd != NETWORK_INVALID_SOCKET) {
    close(server->fd);
  }

  free(server);
}

network_connection_t *network_connect(const char *host, uint16_t port) {
  if (host == NULL) {
    set_error(NETWORK_ERROR_INVALID_ARGUMENT);
    return NULL;
  }

  char port_string[NETWORK_SERVICE_SIZE];

  int written =
      snprintf(port_string, sizeof(port_string), "%u", (unsigned)port);

  if (written < 0 || (size_t)written >= sizeof(port_string)) {
    set_error(NETWORK_ERROR_INVALID_ARGUMENT);
    return NULL;
  }

  struct addrinfo hints;
  struct addrinfo *results = NULL;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int result = getaddrinfo(host, port_string, &hints, &results);

  if (result != 0) {
    set_error(NETWORK_ERROR_CONNECT);
    return NULL;
  }

  int connected_fd = NETWORK_INVALID_SOCKET;

  for (struct addrinfo *address = results; address != NULL;
       address = address->ai_next) {
    int fd = create_tcp_socket(address);

    if (fd == NETWORK_INVALID_SOCKET) {
      continue;
    }

    if (connect(fd, address->ai_addr, address->ai_addrlen) == 0) {
      connected_fd = fd;
      break;
    }

    close(fd);
  }

  freeaddrinfo(results);

  if (connected_fd == NETWORK_INVALID_SOCKET) {
    set_error(NETWORK_ERROR_CONNECT);
    return NULL;
  }

  network_connection_t *connection = malloc(sizeof(*connection));

  if (connection == NULL) {
    close(connected_fd);
    set_error(NETWORK_ERROR_SYSTEM);
    return NULL;
  }

  connection->fd = connected_fd;

  set_error(NETWORK_OK);

  return connection;
}

bool network_send(network_connection_t *connection, const void *data,
                  size_t length) {
  if (connection == NULL || (data == NULL && length != 0)) {
    set_error(NETWORK_ERROR_INVALID_ARGUMENT);
    return false;
  }

  const uint8_t *bytes = data;
  size_t sent = 0;

  while (sent < length) {
    ssize_t result = send(connection->fd, bytes + sent, length - sent, 0);

    if (result > 0) {
      sent += (size_t)result;
      continue;
    }

    if (result == 0) {
      set_error(NETWORK_ERROR_CLOSED);
      return false;
    }

    if (errno == EINTR) {
      continue;
    }

    set_error(NETWORK_ERROR_SEND);
    return false;
  }

  set_error(NETWORK_OK);

  return true;
}

size_t network_receive(network_connection_t *connection, void *buffer,
                       size_t length) {
  if (connection == NULL || (buffer == NULL && length != 0)) {
    set_error(NETWORK_ERROR_INVALID_ARGUMENT);
    return SIZE_MAX;
  }

  if (length == 0) {
    set_error(NETWORK_OK);
    return 0;
  }

  for (;;) {
    ssize_t result = recv(connection->fd, buffer, length, 0);

    if (result > 0) {
      set_error(NETWORK_OK);
      return (size_t)result;
    }

    if (result == 0) {
      set_error(NETWORK_ERROR_CLOSED);
      return false;
    }

    if (errno == EINTR) {
      continue;
    }

    set_error(NETWORK_ERROR_RECEIVE);
    return SIZE_MAX;
  }
}

void network_connection_destroy(network_connection_t *connection) {
  if (connection == NULL) {
    return;
  }

  if (connection->fd != NETWORK_INVALID_SOCKET) {
    close(connection->fd);
  }

  free(connection);
}

network_error_t network_last_error(void) { return g_last_error; }

const char *network_error_string(network_error_t error) {
  switch (error) {
  case NETWORK_OK:
    return "success";
  case NETWORK_ERROR_INVALID_ARGUMENT:
    return "invalid argument";
  case NETWORK_ERROR_SOCKET:
    return "socket error";
  case NETWORK_ERROR_BIND:
    return "bind error";
  case NETWORK_ERROR_LISTEN:
    return "listen error";
  case NETWORK_ERROR_ACCEPT:
    return "accept error";
  case NETWORK_ERROR_CONNECT:
    return "connect error";
  case NETWORK_ERROR_SEND:
    return "send error";
  case NETWORK_ERROR_RECEIVE:
    return "receive error";
  case NETWORK_ERROR_CLOSED:
    return "connection closed";
  case NETWORK_ERROR_SYSTEM:
    return "system error";
  default:
    return "unknown network error";
  }
}
