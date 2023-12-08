#include "kvs.h"

#include <stdlib.h>
#include <string.h>

#include <hashmap.h>

struct kvs {
  struct hashmap *hm;
};

static void cleanup_entry(struct stored_data const *const sd) {
  if (sd && sd->key) {
    free((void *)sd->key);
  }
}

void kvs_delete(struct kvs *kvs, char const *const key, size_t const key_len) {
  struct stored_data const *const sd =
      hashmap_delete(kvs->hm, &(struct stored_data const){.key = key, .key_len = key_len});
  cleanup_entry(sd);
}

void *kvs_set(struct kvs *kvs,
              char const *const key,
              size_t const key_len,
              int32_t const width,
              int32_t const height,
              uint64_t used_at,
              size_t const p_len) {
  void *ptr = malloc(key_len + p_len + 32);
  if (!ptr) {
    return NULL;
  }
  memcpy(ptr, key, key_len);
  void *ptr2 = (void *)(((intptr_t)(ptr) + key_len + 31) & ~31);
  struct stored_data const sd = {
      .key = ptr,
      .key_len = key_len,
      .width = width,
      .height = height,
      .used_at = used_at,
      .p = ptr2,
  };

  uint64_t const hash = hashmap_hash(kvs->hm, &sd);
  struct stored_data const *const deleted =
      hashmap_delete_with_hash(kvs->hm, &(struct stored_data const){.key = key, .key_len = key_len}, hash);
  cleanup_entry(deleted);

  struct stored_data *r = (void *)(hashmap_set_with_hash(kvs->hm, &sd, hash));
  if (!r && hashmap_oom(kvs->hm)) {
    free(ptr);
    return NULL;
  }
  return ptr2;
}

struct stored_data *kvs_get(struct kvs *kvs, const char *const key, const size_t key_len) {
  return (void *)(hashmap_get(kvs->hm, &(struct stored_data const){.key = key, .key_len = key_len}));
}

static void *my_realloc(void *ptr, size_t sz, void *udata) {
  (void)udata;
  return realloc(ptr, sz);
}

static void my_free(void *ptr, void *udata) {
  (void)udata;
  free(ptr);
}

static uint64_t hash(void const *const item, uint64_t const seed0, uint64_t const seed1, void *const udata) {
  struct stored_data const *const v = item;
  (void)udata;
  return hashmap_sip(v->key, v->key_len, seed0, seed1);
}

static int compare(void const *const a, void const *const b, void *const udata) {
  struct stored_data const *const va = a;
  struct stored_data const *const vb = b;
  (void)udata;
  int const r = memcmp(va->key, vb->key, va->key_len < vb->key_len ? va->key_len : vb->key_len);
  if (r != 0) {
    return r;
  }
  if (va->key_len < vb->key_len) {
    return -1;
  }
  if (va->key_len > vb->key_len) {
    return 1;
  }
  return 0;
}

static uint64_t next(uint64_t *const x) {
  uint64_t z = (*x += 0x9e3779b97f4a7c15);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
  z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
  return z ^ (z >> 31);
}

struct kvs *kvs_init(uint64_t const seed) {
  struct kvs *const kvs = malloc(sizeof(struct kvs));
  if (!kvs) {
    return NULL;
  }
  uint64_t x = seed;
  kvs->hm = hashmap_new_with_allocator(
      my_realloc, my_free, sizeof(struct stored_data), 8, next(&x), next(&x), hash, compare, NULL, NULL);
  if (!kvs->hm) {
    free(kvs);
    return NULL;
  }
  return kvs;
}

void kvs_destroy(struct kvs *const kvs) {
  if (!kvs) {
    return;
  }
  size_t iter = 0;
  void *item;
  while (hashmap_iter(kvs->hm, &iter, &item)) {
    cleanup_entry(item);
  }
  hashmap_free(kvs->hm);
  kvs->hm = NULL;
  free(kvs);
}
