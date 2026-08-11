#include "hashmap.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_create_destroy(void) {
  hashmap_t *map = hashmap_create(16);

  assert(map != NULL);
  assert(hashmap_size(map) == 0);

  hashmap_destroy(map);
}

static void test_set_get(void) {
  hashmap_t *map = hashmap_create(16);

  assert(map != NULL);

  assert(hashmap_set(map, "name", "kvstore"));
  assert(hashmap_size(map) == 1);

  const char *value = hashmap_get(map, "name");

  assert(value != NULL);
  assert(strcmp(value, "kvstore") == 0);

  hashmap_destroy(map);
}

static void test_update(void) {
  hashmap_t *map = hashmap_create(16);

  assert(hashmap_set(map, "name", "kvstore"));
  assert(hashmap_set(map, "name", "kvs"));

  const char *value = hashmap_get(map, "name");

  assert(value != NULL);
  assert(strcmp(value, "kvs") == 0);

  hashmap_destroy(map);
}

static void test_missing_key(void) {
  hashmap_t *map = hashmap_create(16);

  assert(hashmap_get(map, "name") == NULL);
  assert(!hashmap_contains(map, "random"));

  hashmap_destroy(map);
}

static void test_contains(void) {
  hashmap_t *map = hashmap_create(16);

  assert(hashmap_set(map, "name", "kvstore"));

  assert(hashmap_contains(map, "name"));
  assert(!hashmap_contains(map, "random"));

  hashmap_destroy(map);
}

static void test_delete(void) {
  hashmap_t *map = hashmap_create(16);

  assert(hashmap_set(map, "name", "kvstore"));
  assert(hashmap_size(map) == 1);

  assert(hashmap_delete(map, "name"));
  assert(hashmap_size(map) == 0);

  assert(hashmap_get(map, "name") == NULL);
  assert(!hashmap_contains(map, "name"));

  hashmap_destroy(map);
}

static void test_delete_missing_key(void) {
  hashmap_t *map = hashmap_create(16);

  assert(!hashmap_delete(map, "random"));
  assert(hashmap_size(map) == 0);

  hashmap_destroy(map);
}

static void test_multiple_entries(void) {
  hashmap_t *map = hashmap_create(16);

  assert(hashmap_set(map, "name", "x"));
  assert(hashmap_set(map, "language", "C"));
  assert(hashmap_set(map, "project", "kvstore"));

  assert(hashmap_size(map) == 3);

  assert(strcmp(hashmap_get(map, "name"), "x") == 0);
  assert(strcmp(hashmap_get(map, "language"), "C") == 0);
  assert(strcmp(hashmap_get(map, "project"), "kvstore") == 0);

  hashmap_destroy(map);
}

static void test_resize(void) {
  hashmap_t *map = hashmap_create(2);

  assert(map != NULL);

  for (int i = 0; i < 101; i++) {
    char key[32];
    char value[32];

    snprintf(key, sizeof(key), "key-%d", i);
    snprintf(value, sizeof(value), "value-%d", i);

    assert(hashmap_set(map, key, value));
  }

  assert(hashmap_size(map) == 101);

  for (int i = 0; i < 101; i++) {
    char key[32];
    char expected_value[32];

    snprintf(key, sizeof(key), "key-%d", i);
    snprintf(expected_value, sizeof(expected_value), "value-%d", i);

    const char *value = hashmap_get(map, key);

    assert(strcmp(value, expected_value) == 0);
  }

  hashmap_destroy(map);
}

int main(void) {
  test_create_destroy();
  test_set_get();
  test_update();
  test_missing_key();
  test_contains();
  test_delete();
  test_delete_missing_key();
  test_multiple_entries();
  test_resize();

  return 0;
}
