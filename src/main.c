#include <stdio.h>

#include <kvstore/hashmap.h>

int main(void) {
  hashmap_t *map = hashmap_create(0);

  if (map == NULL) {
    fprintf(stderr, "Error: hashmap_create\n");
    return 1;
  }

  hashmap_set(map, "name", "kvstore");

  printf("Size: %zu\n", hashmap_size(map));

  const char *name = hashmap_get(map, "name");

  if (name != NULL) {
    printf("%s\n", name);
  }

  hashmap_destroy(map);

  return 0;
}
