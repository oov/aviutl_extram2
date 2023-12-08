#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct kvs;

struct stored_data {
  void const *key;
  size_t key_len;

  int32_t width;
  int32_t height;
  uint64_t used_at;
  void const *p;
};

struct kvs *kvs_init(uint64_t const seed);
void kvs_destroy(struct kvs *kvs);

void kvs_delete(struct kvs *kvs, char const *const key, size_t const key_len);
void *kvs_set(struct kvs *kvs,
              char const *const key,
              size_t const key_len,
              int32_t const width,
              int32_t const height,
              uint64_t used_at,
              size_t const p_len);
struct stored_data *kvs_get(struct kvs *kvs, const char *const key, const size_t key_len);
