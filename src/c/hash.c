#include "hash.h"

#include <stddef.h>
#include <stdint.h>

static uint64_t cyrb64(uint32_t const *const src, size_t const len, uint32_t const seed) {
  uint32_t h1 = 0x91eb9dc7 ^ seed, h2 = 0x41c6ce57 ^ seed;
  for (size_t i = 0; i < len; ++i) {
    h1 = (h1 ^ src[i]) * 2654435761;
    h2 = (h2 ^ src[i]) * 1597334677;
  }
  h1 = ((h1 ^ (h1 >> 16)) * 2246822507) ^ ((h2 ^ (h2 >> 13)) * 3266489909);
  h2 = ((h2 ^ (h2 >> 16)) * 2246822507) ^ ((h1 ^ (h1 >> 13)) * 3266489909);
  return (((uint64_t)h2) << 32) | ((uint64_t)h1);
}

static inline void to_hex(char *const dst, uint64_t x) {
  const char *chars = "0123456789abcdef";
  for (int i = 15; i >= 0; --i) {
    dst[i] = chars[x & 0xf];
    x >>= 4;
  }
}

int calc_hash(lua_State *L) {
  void const *const p = lua_touserdata(L, 1);
  int const w = lua_tointeger(L, 2);
  int const h = lua_tointeger(L, 3);
  if (!p || w <= 0 || h <= 0) {
    lua_pushstring(L, "invalid arguments");
    return lua_error(L);
  }
  char b[16];
  to_hex(b, cyrb64(p, (size_t)(w * h), 0x3fc0b49e));
  lua_pushlstring(L, b, 16);
  return 1;
}
