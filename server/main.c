#include "server.h"

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_PORT 6363
#define DEFAULT_WAL_PATH "kvstore.wal"

int main(int argc, char **argv) {
  const char *wal_path = DEFAULT_WAL_PATH;
  const char *host = NULL;
  uint16_t port = DEFAULT_PORT;
  server_t *server;
  int result;

  if (argc > 1) {
    wal_path = argv[1];
  }

  if (argc > 2) {
    char *end = NULL;
    long parsed_port = strtol(argv[2], &end, 10);

    if (end == argv[2] || *end != '\0' || parsed_port <= 0 ||
        parsed_port >= 65535) {
      fprintf(stderr, "main: invalid port '%s'\n", argv[2]);
      return EXIT_FAILURE;
    }

    port = (uint16_t)parsed_port;
  }

  if (argc > 3) {
    host = argv[3];
  }

  server = server_create(host, port, wal_path);

  if (server == NULL) {
    fprintf(stderr, "main: failed to start server\n");
    return EXIT_FAILURE;
  }

  fprintf(stderr, "main: kvstore-server listening on port %u ( wal=%s )\n",
          (unsigned)port, wal_path);

  result = server_run(server);

  server_destroy(server);

  return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
