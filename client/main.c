#include <kvstore/client.h>

#include <kvstore/protocol.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define DEFAULT_PORT 6363
#define MAX_LINE 4096

static const char *status_name(protocol_status_t status) {
  switch (status) {
  case PROTOCOL_STATUS_OK:
    return "OK";
  case PROTOCOL_STATUS_NOT_FOUND:
    return "NOT_FOUND";
  case PROTOCOL_STATUS_INVALID:
    return "INVALID";
  case PROTOCOL_STATUS_ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

static void print_value(const uint8_t *value, uint32_t value_length) {
  fwrite(value, 1, value_length, stdout);
  fputc('\n', stdout);
}

static int run_set(client_t *client, const char *key, const char *value) {
  protocol_status_t status;

  int result =
      client_set(client, (const uint8_t *)key, (uint32_t)strlen(key),
                 (const uint8_t *)value, (uint32_t)strlen(value), &status);

  if (result != 0) {
    fprintf(stderr, "SET failed: %s\n", client_last_error_string(client));
    return -1;
  }

  printf("%s\n", status_name(status));

  return 0;
}

static int run_get(client_t *client, const char *key) {
  protocol_status_t status;
  uint8_t *value = NULL;
  uint32_t value_length = 0;

  int result = client_get(client, (const uint8_t *)key, (uint32_t)strlen(key),
                          &status, &value, &value_length);

  if (result != 0) {
    fprintf(stderr, "GET failed: %s\n", client_last_error_string(client));
    return -1;
  }

  if (status == PROTOCOL_STATUS_OK) {
    print_value(value, value_length);
    free(value);
  } else {
    printf("%s\n", status_name(status));
  }

  return 0;
}

static int run_del(client_t *client, const char *key) {
  protocol_status_t status;

  int result =
      client_del(client, (const uint8_t *)key, (uint32_t)strlen(key), &status);

  if (result != 0) {
    fprintf(stderr, "DEL failed: %s\n", client_last_error_string(client));
    return -1;
  }

  printf("%s\n", status_name(status));

  return 0;
}

/*
 * Dispatched one already-tokenized command.
 */
static int dispatch(client_t *client, const char *command, const char *key,
                    const char *value) {
  if (strcasecmp(command, "SET") == 0) {
    if (key == NULL || value == NULL) {
      fprintf(stderr, "usage: SET <key> <value>\n");
      return -1;
    }
    return run_set(client, key, value);
  }

  if (strcasecmp(command, "GET") == 0) {
    if (key == NULL) {
      fprintf(stderr, "usage: GET <key>\n");
      return -1;
    }
    return run_get(client, key);
  }

  if (strcasecmp(command, "DEL") == 0) {
    if (key == NULL) {
      fprintf(stderr, "usage: DEL <key>\n");
      return -1;
    }
    return run_del(client, key);
  }

  fprintf(stderr, "unknown command '%s' ( expected SET, GET, or DEL )\n",
          command);
  return -1;
}

/*
 * Interactive REPL: reads lines of the form
 * 	SET <key> <value...>
 * 	GET <key>
 * 	DEL <key>
 * 	QUIT | EXIT
 * from stdin until EOF or QUIT/EXIT.
 */
static void run_repl(client_t *client) {
  char line[MAX_LINE];

  printf("kvstore-client connected. Commands: SET <key> <value>, GET <key>, "
         "DEL <key>, QUIT\n");

  while (true) {
    printf("> ");
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) == NULL) {
      printf("\n");
      break;
    }

    line[strcspn(line, "\n")] = '\0';

    if (line[0] == '\0') {
      continue;
    }

    char *command = strtok(line, " \t");

    if (command == NULL) {
      continue;
    }

    if (strcasecmp(command, "QUIT") == 0 || strcasecmp(command, "EXIT") == 0) {
      break;
    }

    char *key = strtok(NULL, " \t");
    /* Everything after the key, is the value. */
    char *value = strtok(NULL, "");

    if (value != NULL) {
      /* Trim a single leading space left by the delimiter split. */
      while (*value == ' ' || *value == '\t') {
        value++;
      }
    }

    dispatch(client, command, key, value);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr,
            "usage: %s <host> [port] [SET <key> <value> | GET <key> | DEL "
            "<key>]\n"
            "	%s <host> [port]	( interactive mode )\n",
            argv[0], argv[0]);
    return EXIT_FAILURE;
  }

  const char *host = argv[1];
  uint16_t port = DEFAULT_PORT;
  int argi = 2;

  if (argc > 2) {
    char *end = NULL;
    long parsed_port = strtol(argv[2], &end, 10);

    if (end != argv[2] && *end == '\0' && parsed_port >= 1 &&
        parsed_port <= 65535) {
      port = (uint16_t)parsed_port;
      argi = 3;
    }
  }

  client_t *client = client_connect(host, port);

  if (client == NULL) {
    return EXIT_FAILURE;
  }

  int result = EXIT_SUCCESS;

  if (argi >= argc) {
    /* No command given: interactive mode. */
    run_repl(client);
  } else {
    const char *command = argv[argi];
    const char *key = (argi + 1 < argc) ? argv[argi + 1] : NULL;

    char value_buffer[MAX_LINE];
    const char *value = NULL;

    if (argi + 2 < argc) {
      value_buffer[0] = '\0';
      for (int i = argi + 2; i < argc; i++) {
        if (i > argi + 2) {
          strncat(value_buffer, " ",
                  sizeof(value_buffer) - strlen(value_buffer) - 1);
        }
        strncat(value_buffer, argv[i],
                sizeof(value_buffer) - strlen(value_buffer) - 1);
      }
      value = value_buffer;
    }

    if (dispatch(client, command, key, value) != 0) {
      result = EXIT_FAILURE;
    }
  }

  client_disconnect(client);

  return result;
}
