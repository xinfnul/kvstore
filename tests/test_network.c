#include "network.h"
#include <kvstore/network.h>

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_PORT 19090

typedef struct {
  network_server_t *server;
  int success;
} server_context_t;

static void *server_thread(void *arg) {
  server_context_t *context = arg;

  network_connection_t *connection = network_server_accept(context->server);

  assert(connection != NULL);

  const char expected_request[] = "hello from client";
  char buffer[sizeof(expected_request)] = {0};

  size_t received =
      network_receive(connection, buffer, sizeof(expected_request) - 1);

  assert(received == sizeof(expected_request) - 1);
  assert(memcmp(buffer, expected_request, sizeof(expected_request) - 1) == 0);

  const char response[] = "hello from server";

  assert(network_send(connection, response, sizeof(response) - 1));

  network_connection_destroy(connection);

  context->success = 1;

  return NULL;
}

static void test_network_server_client(void) {
  network_server_t *server =
      network_server_create("127.0.0.1", TEST_PORT, NETWORK_DEFAULT_BACKLOG);

  assert(server != NULL);

  server_context_t context = {.server = server, .success = 0};

  pthread_t thread;

  int result = pthread_create(&thread, NULL, server_thread, &context);

  assert(result == 0);

  network_connection_t *client = network_connect("127.0.0.1", TEST_PORT);

  assert(client != NULL);

  const char request[] = "hello from client";

  assert(network_send(client, request, sizeof(request) - 1));

  const char expected_response[] = "hello from server";

  char buffer[sizeof(expected_response)] = {0};

  size_t received =
      network_receive(client, buffer, sizeof(expected_response) - 1);

  assert(received == sizeof(expected_response) - 1);

  assert(memcmp(buffer, expected_response, sizeof(expected_response) - 1) == 0);

  network_connection_destroy(client);

  result = pthread_join(thread, NULL);

  assert(result == 0);
  assert(context.success == 1);

  network_server_destroy(server);
}

static void test_network_error_strings(void) {
  assert(strcmp(network_error_string(NETWORK_OK), "success") == 0);

  assert(strcmp(network_error_string(NETWORK_ERROR_CONNECT), "connect error") ==
         0);

  assert(strcmp(network_error_string(NETWORK_ERROR_CLOSED),
                "connection closed") == 0);
}

int main(void) {
  printf("\033[36mRunning network tests...\033[0m\n");

  test_network_server_client();
  printf("server/client communication: \033[32mOK\033[0m\n");

  test_network_error_strings();
  printf("error strings: \033[32mOK\033[0m\n");

  printf("\033[32mAll network tests passed.\033[0m\n");

  return 0;
}
