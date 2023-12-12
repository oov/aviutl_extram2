#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct kvs;

struct stored_data {
  void const *key;
  size_t key_len;

  uint32_t width;
  uint32_t height;
  uint64_t used_at;
  void const *p;
};

struct kvs *kvs_init(uint64_t const seed0, uint64_t const seed1);
void kvs_destroy(struct kvs *const kvs);

void kvs_delete(struct kvs *const kvs, void const *const key, size_t const key_len);
void *kvs_set(struct kvs *const kvs,
              void const *const key,
              size_t const key_len,
              uint32_t const width,
              uint32_t const height,
              uint64_t const used_at,
              size_t const p_len);
struct stored_data *kvs_get(struct kvs *const kvs, void const *const key, size_t const key_len);
