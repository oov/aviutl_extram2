#include "kvs.h"

#include <stdio.h>
#include <string.h>

int main(void) {
  struct kvs *kvs = kvs_init(1234);
  if (!kvs) {
    printf("kvs_init failed\n");
    return 1;
  }
  if (kvs_get(kvs, "hello world", 11) != NULL) {
    printf("kvs_get should have failed\n");
    goto cleanup;
  }
  void *ptr = kvs_set(kvs, "hello world", 11, 1234, 2345, 3456, 13);
  if (!ptr) {
    printf("kvs_set failed\n");
    goto cleanup;
  }
  memcpy(ptr, "goodbye world", 13);
  struct stored_data const *sd2 = kvs_get(kvs, "hello world", 11);
  if (!sd2) {
    printf("kvs_get failed\n");
    goto cleanup;
  }
  if (sd2->width != 1234 || sd2->height != 2345 || sd2->used_at != 3456) {
    printf("wrong metadata\n");
    goto cleanup;
  }
  if (memcmp(sd2->p, "goodbye world", 13) == 0) {
    printf("wrong data\n");
    goto cleanup;
  }
  kvs_delete(kvs, "hello world", 11);
  if (kvs_get(kvs, "hello world", 11) != NULL) {
    printf("kvs_get should have failed\n");
    goto cleanup;
  }
cleanup:
  kvs_destroy(kvs);
  return 0;
}
