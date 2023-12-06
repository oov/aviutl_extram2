#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct stored_data {
  void const *key;
  size_t key_len;

  int32_t width;
  int32_t height;
  uint64_t used_at;
  void const *p;
};

bool kvs_init(uint64_t const seed);
void kvs_destroy(void);

void kvs_delete(char const *const key, size_t const key_len);
bool kvs_set(char const *const key,
             size_t const key_len,
             int32_t const width,
             int32_t const height,
             uint64_t used_at,
             void const *const p,
             size_t const p_len);
struct stored_data *kvs_get(const char *const key, const size_t key_len);
