#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "kvs.h"

struct userdata {
  struct kvs *kvs;
};

struct pixel {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;
};

static inline size_t zumin(size_t const a, size_t const b) { return a < b ? a : b; }

static struct userdata *get_userdata(lua_State *L, int const idx) { return lua_touserdata(L, idx); }

static bool get_direct(struct kvs *kvs,
                       char const *const key,
                       size_t const key_len,
                       struct pixel *d,
                       size_t const dstw,
                       size_t const dsth) {
  struct stored_data const *const sd = kvs_get(kvs, key, key_len);
  if (!sd || sd->width == 0 || sd->height == 0 || sd->p == NULL) {
    return false;
  }

  size_t const srcw = (size_t)sd->width;
  size_t const srch = (size_t)sd->height;
  struct pixel const *s = sd->p;

  if (srcw == dstw && srch == dsth) {
    memcpy(d, s, srcw * sizeof(struct pixel) * srch);
    return true;
  }

  size_t const w = zumin(srcw, dstw) * sizeof(struct pixel);
  size_t const h = zumin(srch, dsth);
  for (size_t y = 0; y < h; ++y) {
    memcpy(d, s, w);
    s += srcw;
    d += dstw;
  }
  return true;
}

static bool set_direct(struct kvs *kvs,
                       char const *const key,
                       size_t const key_len,
                       struct pixel const *s,
                       size_t const srcw,
                       size_t const srch) {
  void *ptr = kvs_set(kvs, key, key_len, srcw, srch, GetTickCount64(), srcw * sizeof(struct pixel) * srch);
  if (!ptr) {
    return false;
  }
  memcpy(ptr, s, srcw * sizeof(struct pixel) * srch);
  return true;
}

static int luafn_del(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = get_userdata(L, 1);
  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  if (!ud || !key || !key_len) {
    return luaL_error(L, "invalid arguments");
  }
  kvs_delete(ud->kvs, key, key_len);
  return 0;
}

static int luafn_get_direct(lua_State *L) {
  if (lua_gettop(L) < 5) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = get_userdata(L, 1);
  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  struct pixel *d = lua_touserdata(L, 3);
  size_t const dstw = lua_tointeger(L, 4);
  size_t const dsth = lua_tointeger(L, 5);
  if (!ud || !key || !key_len || !d || !dstw || !dsth) {
    return luaL_error(L, "invalid arguments");
  }
  lua_pushboolean(L, get_direct(ud->kvs, key, key_len, d, dstw, dsth));
  return 1;
}

static int luafn_get(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = get_userdata(L, 1);
  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  if (!ud || !key || !key_len) {
    return luaL_error(L, "invalid arguments");
  }
  lua_getglobal(L, "obj");
  lua_getfield(L, -1, "w");
  lua_getfield(L, -2, "h");
  if (lua_tointeger(L, -1) == 0 || lua_tointeger(L, -2) == 0) {
    return luaL_error(L, "has no image");
  }
  lua_pop(L, 2);
  lua_getfield(L, -1, "getpixeldata");
  lua_call(L, 0, 3);
  struct pixel *d = lua_touserdata(L, -3);
  size_t const dstw = lua_tointeger(L, -2);
  size_t const dsth = lua_tointeger(L, -1);
  if (!get_direct(ud->kvs, key, key_len, d, dstw, dsth)) {
    lua_pushboolean(L, false);
    return 1;
  }
  lua_pop(L, 2);
  lua_getfield(L, -2, "putpixeldata");
  lua_pushvalue(L, -2);
  lua_call(L, 1, 0);
  lua_pushboolean(L, true);
  return 1;
}

static int luafn_set_direct(lua_State *L) {
  if (lua_gettop(L) < 5) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = get_userdata(L, 1);
  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  struct pixel *d = lua_touserdata(L, 3);
  size_t const srcw = lua_tointeger(L, 4);
  size_t const srch = lua_tointeger(L, 5);
  if (!ud || !key || !key_len || !d || !srcw || !srch) {
    return luaL_error(L, "invalid arguments");
  }
  lua_pushboolean(L, set_direct(ud->kvs, key, key_len, d, srcw, srch));
  return 1;
}

static int luafn_get_size(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = get_userdata(L, 1);
  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  if (!ud || !key || !key_len) {
    return luaL_error(L, "invalid arguments");
  }
  struct stored_data const *const sd = kvs_get(ud->kvs, key, key_len);
  if (!sd || sd->width == 0 || sd->height == 0 || sd->p == NULL) {
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
  }
  lua_pushinteger(L, sd->width);
  lua_pushinteger(L, sd->height);
  return 2;
}

static int luafn_set(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = get_userdata(L, 1);
  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  if (!ud || !key || !key_len) {
    return luaL_error(L, "invalid arguments");
  }
  lua_getglobal(L, "obj");
  lua_getfield(L, -1, "w");
  lua_getfield(L, -2, "h");
  if (lua_tointeger(L, -1) == 0 || lua_tointeger(L, -2) == 0) {
    return luaL_error(L, "has no image");
  }
  lua_pop(L, 2);
  lua_getfield(L, -1, "getpixeldata");
  lua_call(L, 0, 3);
  struct pixel *d = lua_touserdata(L, -3);
  size_t const srcw = lua_tointeger(L, -2);
  size_t const srch = lua_tointeger(L, -1);
  if (!set_direct(ud->kvs, key, key_len, d, srcw, srch)) {
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
  }
  lua_pushinteger(L, srcw);
  lua_pushinteger(L, srch);
  return 2;
}

static int luafn_get_str(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = get_userdata(L, 1);
  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  if (!ud || !key || !key_len) {
    return luaL_error(L, "invalid arguments");
  }
  struct stored_data const *const sd = kvs_get(ud->kvs, key, key_len);
  if (!sd) {
    lua_pushnil(L);
    return 1;
  }
  lua_pushlstring(L, sd->p, sd->height);
  return 1;
}

static int luafn_set_str(lua_State *L) {
  if (lua_gettop(L) < 3) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = get_userdata(L, 1);
  size_t key_len, value_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  char const *const value = lua_tolstring(L, 3, &value_len);
  if (!ud || !key || !key_len || !value || !value_len) {
    return luaL_error(L, "invalid arguments");
  }
  void *ptr = kvs_set(ud->kvs, key, key_len, 0, value_len, GetTickCount64(), value_len);
  if (ptr) {
    memcpy(ptr, value, value_len);
  }
  lua_pushboolean(L, ptr != NULL);
  return 1;
}

static int finalize(lua_State *L) {
  if (lua_gettop(L) < 1) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = get_userdata(L, 1);
  if (!ud) {
    return luaL_error(L, "invalid arguments");
  }
  kvs_destroy(ud->kvs);
  ud->kvs = NULL;
  return 0;
}

int __declspec(dllexport) luaopen_Intram2(lua_State *L);
int __declspec(dllexport) luaopen_Intram2(lua_State *L) {
  static char const name[] = "Intram2";
  static char const meta_name[] = "Intram2_Meta";
  static luaL_Reg const funcs[] = {
      {"del", luafn_del},
      {"get", luafn_get},
      {"get_direct", luafn_get_direct},
      {"get_size", luafn_get_size},
      {"set", luafn_set},
      {"set_direct", luafn_set_direct},
      {"get_str", luafn_get_str},
      {"set_str", luafn_set_str},
      {NULL, NULL},
  };
  luaL_newmetatable(L, meta_name);
  lua_pushstring(L, "__index");
  lua_newtable(L);
  luaL_register(L, NULL, funcs);
  lua_settable(L, -3);
  lua_pushstring(L, "__gc");
  lua_pushcfunction(L, finalize);
  lua_settable(L, -3);
  lua_pop(L, 1);

  struct kvs *kvs = kvs_init(GetTickCount64());
  if (!kvs) {
    return luaL_error(L, "allocation failed");
  }

  struct userdata *ud = lua_newuserdata(L, sizeof(struct userdata));
  if (!ud) {
    return luaL_error(L, "lua_newuserdata failed");
  }
  ud->kvs = kvs;
  lua_pushvalue(L, -1);
  luaL_getmetatable(L, meta_name);
  lua_setmetatable(L, -2);
  lua_setglobal(L, name);
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaded");
  lua_pushvalue(L, -3);
  lua_setfield(L, -2, name);
  lua_pop(L, 2);
  return 1;
}
