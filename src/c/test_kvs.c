#include "kvs.h"

#include <stdio.h>
#include <string.h>

int main(void) {
  if (!kvs_init(1234)) {
    printf("kvs_init failed\n");
    return 1;
  }
  if (kvs_get("hello world", 11) != NULL) {
    printf("kvs_get should have failed\n");
    goto cleanup;
  }
  if (!kvs_set("hello world", 11, 1234, 2345, 3456, "goodbye world", 13)) {
    printf("kvs_set failed\n");
    goto cleanup;
  }
  struct stored_data const *sd = kvs_get("hello world", 11);
  if (sd == NULL) {
    printf("kvs_get failed\n");
    goto cleanup;
  }
  if (sd->width != 1234 || sd->height != 2345 || sd->used_at != 3456) {
    printf("wrong metadata\n");
    goto cleanup;
  }
  if (memcmp(sd->p, "goodbye world", 13) == 0) {
    printf("wrong data\n");
    goto cleanup;
  }
  kvs_delete("hello world", 11);
  if (kvs_get("hello world", 11) != NULL) {
    printf("kvs_get should have failed\n");
    goto cleanup;
  }
cleanup:
  kvs_destroy();
  return 0;
}
