#include "kvs.h"

#include <stdlib.h>
#include <string.h>

#include <hashmap.h>

static struct hashmap *g_hm = NULL;

void kvs_delete(char const *const key, size_t const key_len) {
  struct stored_data const *const sd =
      hashmap_delete(g_hm, &(struct stored_data const){.key = key, .key_len = key_len});
  if (sd && sd->key) {
    free((void *)sd->key);
  }
}

bool kvs_set(char const *const key,
             size_t const key_len,
             int32_t const width,
             int32_t const height,
             uint64_t used_at,
             void const *const p,
             size_t const p_len) {
  void *ptr = malloc(key_len + p_len + 32);
  if (!ptr) {
    return false;
  }
  memcpy(ptr, key, key_len);
  void *ptr2 = (void *)(((intptr_t)(ptr) + key_len + 31) & ~31);
  memcpy(ptr2, p, p_len);

  struct stored_data const sd = {
      .key = ptr,
      .key_len = key_len,
      .width = width,
      .height = height,
      .used_at = used_at,
      .p = ptr2,
  };
  kvs_delete(key, key_len);
  if (!hashmap_set(g_hm, &sd) && hashmap_oom(g_hm)) {
    free(ptr);
    return NULL;
  }
  return true;
}

struct stored_data *kvs_get(const char *const key, const size_t key_len) {
  return (void *)(hashmap_get(g_hm, &(struct stored_data const){.key = key, .key_len = key_len}));
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

bool kvs_init(uint64_t const seed) {
  uint64_t x = seed;
  g_hm = hashmap_new_with_allocator(
      my_realloc, my_free, sizeof(struct stored_data), 8, next(&x), next(&x), hash, compare, NULL, NULL);
  return g_hm != NULL;
}

void kvs_destroy(void) {
  size_t iter = 0;
  void *item;
  while (hashmap_iter(g_hm, &iter, &item)) {
    struct stored_data const *const sd = item;
    if (sd && sd->key) {
      free((void *)sd->key);
    }
  }
  hashmap_free(g_hm);
  g_hm = NULL;
}
