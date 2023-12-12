#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "hash.h"
#include "ipc.h"
#include "process.h"
#include "splitmix64.h"

struct userdata {
  wchar_t fmo_name[32];
  struct process *process;
  HANDLE fmo;
  void *view;
  char *buf;
  size_t buf_size;
};

struct pixel {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;
};

struct recv_data {
  lua_State *L;
  char const **error;
  void *userdata;
};

static HMODULE hInst = NULL;

static HANDLE create_fmo(wchar_t const *const base_name, size_t const size, wchar_t *const name32) {
  uint64_t x = GetTickCount64();
  for (int i = 0; i < 5; ++i) {
    wsprintfW(name32, L"%s%08x", base_name, splitmix64(&x));
    HANDLE fmo = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, name32);
    if (fmo) {
      if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(fmo);
        continue;
      }
      return fmo;
    }
  }
  return NULL;
}

static void *grow(void *ptr, size_t *const curlen, size_t const newlen) {
  if (*curlen >= newlen) {
    return ptr;
  }
  void *const newptr = realloc(ptr, newlen);
  if (!newptr) {
    free(ptr);
    return NULL;
  }
  *curlen = newlen;
  return newptr;
}

static wchar_t *get_extram2_path(void) {
  static wchar_t const extram2[] = L"Extram2.exe";
  size_t len = 0;
  wchar_t *path = NULL;
  for (;;) {
    path = grow(path, &len, len + MAX_PATH);
    if (!path) {
      return NULL;
    }
    DWORD n = GetModuleFileNameW(hInst, path, len);
    if (n == 0) {
      free(path);
      return NULL;
    }
    if (n == len) {
      continue;
    }
    break;
  }
  wchar_t *const p = wcsrchr(path, L'\\');
  if (p) {
    p[1] = L'\0';
  }
  path = grow(path, &len, wcslen(path) + wcslen(extram2) + 1);
  if (!path) {
    return NULL;
  }
  wcscat(path, extram2);
  return path;
}

static char const *initialize(lua_State *L, struct userdata *ud) {
  char const *msg = NULL;
  wchar_t *extram2_path = NULL;
  if (ud->fmo == NULL) {
    lua_getglobal(L, "obj");
    lua_getfield(L, -1, "getinfo");
    lua_pushstring(L, "image_max");
    lua_call(L, 1, 2);
    size_t const width = lua_tointeger(L, -2);
    size_t const height = lua_tointeger(L, -1);
    lua_pop(L, 3);
    ud->fmo = create_fmo(L"aviutl_extram_fmo", width * 4 * height, ud->fmo_name);
    if (!ud->fmo) {
      msg = "cannot create file mapping object";
      goto cleanup;
    }
    ud->view = MapViewOfFile(ud->fmo, FILE_MAP_WRITE, 0, 0, 0);
    if (!ud->view) {
      CloseHandle(ud->fmo);
      ud->fmo = NULL;
      msg = "cannot map view of file";
      goto cleanup;
    }
  }
  if (ud->process && !process_isrunning(ud->process)) {
    process_finish(ud->process);
    ud->process = NULL;
  }
  if (ud->process == NULL) {
    extram2_path = get_extram2_path();
    if (!extram2_path) {
      msg = "cannot get path to Extram2.exe";
      goto cleanup;
    }
    ud->process = process_start(extram2_path, L"EXTRAM2_FMO", ud->fmo_name);
    if (!ud->process) {
      msg = "cannot start Extram2.exe";
      goto cleanup;
    }
  }
cleanup:
  if (extram2_path) {
    free(extram2_path);
  }
  return msg;
}

static int finalize(lua_State *L) {
  if (lua_gettop(L) < 1) {
    return luaL_error(L, "invalid number of parameters");
  }
  struct userdata *ud = lua_touserdata(L, 1);
  if (!ud) {
    return luaL_error(L, "invalid arguments");
  }
  if (ud->process) {
    process_finish(ud->process);
  }
  if (ud->fmo) {
    CloseHandle(ud->fmo);
  }
  if (ud->buf) {
    free(ud->buf);
  }
  return 0;
}

static void recv_set(void *const userdata, void const *const ptr, size_t const len) {
  struct recv_data const *const rd = userdata;
  struct cmd_set_resp const *const resp = ptr;
  if (sizeof(*resp) < len) {
    *rd->error = "invalid response size";
    return;
  }
  if (!resp->ok) {
    *rd->error = "failed to store data";
    return;
  }
}

static char const *set_direct(struct userdata *ud,
                              char const *const key,
                              size_t const key_len,
                              struct pixel const *s,
                              size_t const srcw,
                              size_t const srch) {
  struct cmd_set_req const req = {
      .id = CMD_SET,
      .key_len = key_len,
      .width = srcw,
      .height = srch,
  };
  size_t const total_len = sizeof(req) + key_len;
  ud->buf = grow(ud->buf, &ud->buf_size, total_len);
  if (!ud->buf) {
    return "cannot allocate memory";
  }
  memcpy(ud->buf, &req, sizeof(req));
  memcpy(ud->buf + sizeof(req), key, key_len);
  memcpy(ud->view, s, srcw * sizeof(struct pixel) * srch);
  if (process_write(ud->process, ud->buf, total_len)) {
    return "cannot write to Extram2.exe";
  }
  char const *err = NULL;
  if (process_read(ud->process,
                   recv_set,
                   &(struct recv_data){
                       .error = &err,
                   })) {
    return "cannot read from Extram2.exe";
  }
  if (err) {
    return err;
  }
  return NULL;
}

static void recv_del(void *const userdata, void const *const ptr, size_t const len) {
  struct recv_data const *const rd = userdata;
  struct cmd_del_resp const *const resp = ptr;
  if (sizeof(*resp) < len) {
    *rd->error = "invalid response size";
    return;
  }
  if (!resp->ok) {
    *rd->error = "failed to store data";
    return;
  }
}

static int luafn_del(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }

  struct userdata *ud = lua_touserdata(L, 1);
  char const *err = initialize(L, ud);
  if (err) {
    return luaL_error(L, err);
  }

  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  if (!ud || !key || !key_len) {
    return luaL_error(L, "invalid arguments");
  }

  struct cmd_del_req const cmd = {
      .id = CMD_DEL,
      .key_len = key_len,
  };
  size_t const total_len = sizeof(cmd) + key_len;
  ud->buf = grow(ud->buf, &ud->buf_size, total_len);
  memcpy(ud->buf, &cmd, sizeof(cmd));
  memcpy(ud->buf + sizeof(cmd), key, key_len);
  if (process_write(ud->process, ud->buf, total_len)) {
    return luaL_error(L, "cannot write to Extram2.exe");
  }
  if (process_read(ud->process,
                   recv_del,
                   &(struct recv_data){
                       .L = L,
                       .error = &err,
                   })) {
    return luaL_error(L, "cannot read from Extram2.exe");
  }
  if (err) {
    return luaL_error(L, err);
  }
  return 0;
}

static void recv_get(void *const userdata, void const *const ptr, size_t const len) {
  struct recv_data const *const rd = userdata;
  struct cmd_get_resp const *const resp = ptr;
  if (sizeof(*resp) != len) {
    *rd->error = "invalid response size";
    return;
  }
  struct cmd_get_resp *const r = rd->userdata;
  *r = *resp;
}

static int luafn_get_direct(lua_State *L) {
  if (lua_gettop(L) < 5) {
    return luaL_error(L, "invalid number of parameters");
  }

  struct userdata *ud = lua_touserdata(L, 1);
  char const *err = initialize(L, ud);
  if (err) {
    return luaL_error(L, err);
  }

  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  struct pixel *d = lua_touserdata(L, 3);
  size_t const dstw = lua_tointeger(L, 4);
  size_t const dsth = lua_tointeger(L, 5);
  if (!ud || !key || !key_len || !d || !dstw || !dsth) {
    return luaL_error(L, "invalid arguments");
  }

  struct cmd_get_req const req = {
      .id = CMD_GET,
      .key_len = key_len,
  };
  size_t const total_len = sizeof(req) + key_len;
  ud->buf = grow(ud->buf, &ud->buf_size, total_len);
  memcpy(ud->buf, &req, sizeof(req));
  memcpy(ud->buf + sizeof(req), key, key_len);
  if (process_write(ud->process, ud->buf, total_len)) {
    return luaL_error(L, "cannot write to Extram2.exe");
  }
  struct cmd_get_resp resp;
  if (process_read(ud->process,
                   recv_get,
                   &(struct recv_data){
                       .L = L,
                       .error = &err,
                       .userdata = &resp,
                   })) {
    return luaL_error(L, "cannot read from Extram2.exe");
  }
  if (err) {
    return luaL_error(L, err);
  }
  if (!resp.ok || (size_t)resp.width != dstw || (size_t)resp.height != dsth) {
    lua_pushboolean(L, false);
    return 1;
  }
  memcpy(d, ud->view, dstw * sizeof(struct pixel) * dsth);
  lua_pushboolean(L, true);
  return 1;
}

static int luafn_get(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }

  struct userdata *ud = lua_touserdata(L, 1);
  char const *err = initialize(L, ud);
  if (err) {
    return luaL_error(L, err);
  }

  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  if (!ud || !key || !key_len) {
    return luaL_error(L, "invalid arguments");
  }

  struct cmd_get_req const req = {
      .id = CMD_GET,
      .key_len = key_len,
  };
  size_t const total_len = sizeof(req) + key_len;
  ud->buf = grow(ud->buf, &ud->buf_size, total_len);
  memcpy(ud->buf, &req, sizeof(req));
  memcpy(ud->buf + sizeof(req), key, key_len);
  if (process_write(ud->process, ud->buf, total_len)) {
    return luaL_error(L, "cannot write to Extram2.exe");
  }
  struct cmd_get_resp resp;
  if (process_read(ud->process,
                   recv_get,
                   &(struct recv_data){
                       .L = L,
                       .error = &err,
                       .userdata = &resp,
                   })) {
    return luaL_error(L, "cannot read from Extram2.exe");
  }
  if (err) {
    return luaL_error(L, err);
  }
  if (!resp.ok || resp.width == 0 || resp.height == 0) {
    lua_pushboolean(L, false);
    return 1;
  }

  lua_getglobal(L, "obj");
  lua_getfield(L, -1, "setoption");
  lua_pushstring(L, "drawtarget");
  lua_pushstring(L, "tempbuffer");
  lua_pushinteger(L, resp.width);
  lua_pushinteger(L, resp.height);
  lua_call(L, 4, 0);

  lua_getfield(L, -1, "load");
  lua_pushstring(L, "tempbuffer");
  lua_call(L, 1, 0);

  lua_getfield(L, -1, "getpixeldata");
  lua_call(L, 0, 3);
  struct pixel *d = lua_touserdata(L, -3);
  memcpy(d, ud->view, resp.width * sizeof(struct pixel) * resp.height);
  lua_getfield(L, -4, "putpixeldata");
  lua_pushvalue(L, -4);
  lua_call(L, 1, 0);
  lua_pushboolean(L, true);
  return 1;
}

static int luafn_set_direct(lua_State *L) {
  if (lua_gettop(L) < 5) {
    return luaL_error(L, "invalid number of parameters");
  }

  struct userdata *ud = lua_touserdata(L, 1);
  char const *err = initialize(L, ud);
  if (err) {
    return luaL_error(L, err);
  }

  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  struct pixel *d = lua_touserdata(L, 3);
  size_t const srcw = lua_tointeger(L, 4);
  size_t const srch = lua_tointeger(L, 5);
  if (!ud || !key || !key_len || !d || !srcw || !srch) {
    return luaL_error(L, "invalid arguments");
  }

  err = set_direct(ud, key, key_len, d, srcw, srch);
  if (err) {
    return luaL_error(L, err);
  }
  return 0;
}

static int luafn_get_size(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }

  struct userdata *ud = lua_touserdata(L, 1);
  char const *err = initialize(L, ud);
  if (err) {
    return luaL_error(L, err);
  }

  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  if (!ud || !key || !key_len) {
    return luaL_error(L, "invalid arguments");
  }

  struct cmd_get_size_req const req = {
      .id = CMD_GET_SIZE,
      .key_len = key_len,
  };
  size_t const total_len = sizeof(req) + key_len;
  ud->buf = grow(ud->buf, &ud->buf_size, total_len);
  memcpy(ud->buf, &req, sizeof(req));
  memcpy(ud->buf + sizeof(req), key, key_len);
  if (process_write(ud->process, ud->buf, total_len)) {
    return luaL_error(L, "cannot write to Extram2.exe");
  }
  struct cmd_get_size_resp resp;
  if (process_read(ud->process,
                   recv_get,
                   &(struct recv_data){
                       .L = L,
                       .error = &err,
                       .userdata = &resp,
                   })) {
    return luaL_error(L, "cannot read from Extram2.exe");
  }
  if (err) {
    return luaL_error(L, err);
  }
  if (!resp.ok || resp.width == 0 || resp.height == 0) {
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
  }

  lua_pushinteger(L, resp.width);
  lua_pushinteger(L, resp.height);
  return 2;
}

static int luafn_set(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }

  struct userdata *ud = lua_touserdata(L, 1);
  char const *err = initialize(L, ud);
  if (err) {
    return luaL_error(L, err);
  }

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
  err = set_direct(ud, key, key_len, d, srcw, srch);
  if (err) {
    return luaL_error(L, err);
  }
  lua_pushinteger(L, srcw);
  lua_pushinteger(L, srch);
  return 2;
}

static void recv_get_str(void *const userdata, void const *const ptr, size_t const len) {
  struct recv_data const *const rd = userdata;
  struct cmd_get_str_resp const *const resp = ptr;
  if (sizeof(*resp) < len) {
    *rd->error = "invalid response size";
    return;
  }
  if (!resp->ok) {
    lua_pushnil(rd->L);
    return;
  }
  if (sizeof(*resp) + resp->val_len != len) {
    *rd->error = "invalid value size";
    return;
  }
  lua_pushlstring(rd->L, (char const *)(ptr) + sizeof(*resp), resp->val_len);
}

static int luafn_get_str(lua_State *L) {
  if (lua_gettop(L) < 2) {
    return luaL_error(L, "invalid number of parameters");
  }

  struct userdata *ud = lua_touserdata(L, 1);
  char const *err = initialize(L, ud);
  if (err) {
    return luaL_error(L, err);
  }

  size_t key_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  if (!ud || !key || !key_len) {
    return luaL_error(L, "invalid arguments");
  }

  struct cmd_get_str_req const req = {
      .id = CMD_GET_STR,
      .key_len = key_len,
  };
  size_t const total_len = sizeof(req) + key_len;
  ud->buf = grow(ud->buf, &ud->buf_size, total_len);
  memcpy(ud->buf, &req, sizeof(req));
  memcpy(ud->buf + sizeof(req), key, key_len);
  if (process_write(ud->process, ud->buf, total_len)) {
    return luaL_error(L, "cannot write to Extram2.exe");
  }
  if (process_read(ud->process,
                   recv_get_str,
                   &(struct recv_data){
                       .L = L,
                       .error = &err,
                   })) {
    return luaL_error(L, "cannot read from Extram2.exe");
  }
  if (err) {
    return luaL_error(L, err);
  }
  return 1;
}

static void recv_set_str(void *const userdata, void const *const ptr, size_t const len) {
  struct recv_data const *const rd = userdata;
  struct cmd_set_str_resp const *const resp = ptr;
  if (sizeof(*resp) < len) {
    *rd->error = "invalid response size";
    return;
  }
  if (!resp->ok) {
    *rd->error = "failed to store data";
    return;
  }
}

static int luafn_set_str(lua_State *L) {
  if (lua_gettop(L) < 3) {
    return luaL_error(L, "invalid number of parameters");
  }

  struct userdata *ud = lua_touserdata(L, 1);
  char const *err = initialize(L, ud);
  if (err) {
    return luaL_error(L, err);
  }

  size_t key_len, value_len;
  char const *const key = lua_tolstring(L, 2, &key_len);
  char const *const value = lua_tolstring(L, 3, &value_len);
  if (!ud || !key || !key_len || !value || !value_len) {
    return luaL_error(L, "invalid arguments");
  }

  struct cmd_set_str_req const cmd = {
      .id = CMD_SET_STR,
      .key_len = key_len,
      .val_len = value_len,
  };
  size_t const total_len = sizeof(cmd) + key_len + value_len;
  ud->buf = grow(ud->buf, &ud->buf_size, total_len);
  memcpy(ud->buf, &cmd, sizeof(cmd));
  memcpy(ud->buf + sizeof(cmd), key, key_len);
  memcpy(ud->buf + sizeof(cmd) + key_len, value, value_len);
  if (process_write(ud->process, ud->buf, total_len)) {
    return luaL_error(L, "cannot write to Extram2.exe");
  }
  if (process_read(ud->process,
                   recv_set_str,
                   &(struct recv_data){
                       .L = L,
                       .error = &err,
                   })) {
    return luaL_error(L, "cannot read from Extram2.exe");
  }
  if (err) {
    return luaL_error(L, err);
  }
  return 0;
}

int __declspec(dllexport) luaopen_Extram2(lua_State *L);
int __declspec(dllexport) luaopen_Extram2(lua_State *L) {
  struct userdata *ud = lua_newuserdata(L, sizeof(struct userdata));
  if (!ud) {
    return luaL_error(L, "lua_newuserdata failed");
  }
  memset(ud, 0, sizeof(struct userdata));

  static char const name[] = "Extram2";
  static char const meta_name[] = "Extram2_Meta";
  static luaL_Reg const funcs[] = {
      {"del", luafn_del},
      {"get", luafn_get},
      {"get_direct", luafn_get_direct},
      {"get_size", luafn_get_size},
      {"set", luafn_set},
      {"set_direct", luafn_set_direct},
      {"get_str", luafn_get_str},
      {"set_str", luafn_set_str},
      {"calc_hash", calc_hash},
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
  lua_setmetatable(L, -2);

  lua_pushvalue(L, -1);
  lua_setglobal(L, name);
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaded");
  lua_pushvalue(L, -3);
  lua_setfield(L, -2, name);
  lua_pop(L, 2);
  return 1;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  hInst = hinstDLL;
  (void)fdwReason;
  (void)lpvReserved;
  return TRUE;
}
