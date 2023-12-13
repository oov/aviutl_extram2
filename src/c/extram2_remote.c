#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ipc.h"
#include "kvs.h"
#include "splitmix64.h"

// #define EXTRAM2_DEBUG

static struct kvs *g_kvs = NULL;
static HANDLE g_fmo = NULL;
static void *g_view = NULL;

static HANDLE g_stdin = NULL;
static HANDLE g_stdout = NULL;

static void output(void const *const ptr, size_t const len, void const *const ptr2, size_t const len2) {
  uint32_t const len32 = len + len2;
  DWORD written;
  if (!WriteFile(g_stdout, &len32, sizeof(len32), &written, NULL) || written != sizeof(len32)) {
#ifdef EXTRAM2_DEBUG
    OutputDebugStringA("cannot write output data size");
#endif
    return;
  }
  if (len) {
    if (!WriteFile(g_stdout, ptr, len, &written, NULL) || written != (DWORD)len) {
#ifdef EXTRAM2_DEBUG
      OutputDebugStringA("cannot write output data");
#endif
      return;
    }
  }
  if (len2) {
    if (!WriteFile(g_stdout, ptr2, len2, &written, NULL) || written != (DWORD)len2) {
#ifdef EXTRAM2_DEBUG
      OutputDebugStringA("cannot write output data");
#endif
      return;
    }
  }
  if (!FlushFileBuffers(g_stdout)) {
#ifdef EXTRAM2_DEBUG
    OutputDebugStringA("cannot flush output data");
#endif
  }
}

#ifdef EXTRAM2_DEBUG
static void debug_print(char const *const filename,
                        int const line,
                        char const *const func,
                        void const *const key,
                        size_t const key_len,
                        void const *const val,
                        size_t const val_len) {
  char buf[1024];
  char buf2[1024];
  char buf3[1024];

  if (key) {
    memcpy(buf2, key, key_len);
    buf2[key_len] = '\0';
  } else {
    buf2[0] = '\0';
  }

  if (val) {
    memcpy(buf3, val, val_len);
    buf3[val_len] = '\0';
  } else {
    buf3[0] = '\0';
  }
  wsprintfA(buf, "[%s:%d] %s: key: %s val: %s", filename, line, func, buf2, buf3);
  OutputDebugStringA(buf);
}
#  define DBGPRINT(req1, len1, req2, len2) debug_print(__FILE_NAME__, __LINE__, __func__, req1, len1, req2, len2)
#else
#  define DBGPRINT(req1, len1, req2, len2)
#endif

struct pixel {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;
};

static void del(void *const ptr, size_t const len) {
  struct cmd_del_req const *const req = ptr;
  struct cmd_del_resp resp = {0};
  if (len < sizeof(*req) || len != sizeof(*req) + req->key_len) {
    goto cleanup;
  }

  DBGPRINT((void const *)(req + 1), req->key_len, NULL, 0);

  kvs_delete(g_kvs, (void const *)(req + 1), req->key_len);
  resp.ok = true;
cleanup:
  output(&resp, sizeof(resp), NULL, 0);
}

static void get_size(void *const ptr, size_t const len) {
  struct cmd_get_size_req const *const req = ptr;
  struct cmd_get_size_resp resp = {0};
  if (len < sizeof(*req) || len != sizeof(*req) + req->key_len) {
    goto cleanup;
  }

  DBGPRINT((void const *)(req + 1), req->key_len, NULL, 0);

  struct stored_data *const sd = kvs_get(g_kvs, (void const *)(req + 1), req->key_len);
  if (!sd || sd->width == 0 || sd->height == 0 || sd->p == NULL) {
    goto cleanup;
  }

#ifdef EXTRAM2_DEBUG
  {
    char buf[256];
    wsprintfA(buf, "[%s:%d] %s: width: %d height: %d", __FILE_NAME__, __LINE__, __func__, sd->width, sd->height);
    OutputDebugStringA(buf);
  }
#endif

  sd->used_at = GetTickCount64();
  resp.ok = true;
  resp.width = sd->width;
  resp.height = sd->height;
cleanup:
  output(&resp, sizeof(resp), NULL, 0);
}

static void get(void *const ptr, size_t const len) {
  struct cmd_get_req const *const req = ptr;
  struct cmd_get_resp resp = {0};
  if (len < sizeof(*req) || len != sizeof(*req) + req->key_len) {
    goto cleanup;
  }

  struct stored_data *const sd = kvs_get(g_kvs, (void const *)(req + 1), req->key_len);
  if (!sd || sd->width == 0 || sd->height == 0 || sd->p == NULL) {
    goto cleanup;
  }

  DBGPRINT((void const *)(req + 1), req->key_len, NULL, 0);
#ifdef EXTRAM2_DEBUG
  {
    char buf[256];
    wsprintfA(buf, "[%s:%d] %s: width: %d height: %d", __FILE_NAME__, __LINE__, __func__, sd->width, sd->height);
    OutputDebugStringA(buf);
  }
#endif

  memcpy(g_view, sd->p, sd->width * sizeof(struct pixel) * sd->height);
  sd->used_at = GetTickCount64();

  resp.ok = true;
  resp.width = sd->width;
  resp.height = sd->height;
cleanup:
  output(&resp, sizeof(resp), NULL, 0);
}

static void set(void *const ptr, size_t const len) {
  struct cmd_set_req const *const req = ptr;
  struct cmd_set_resp resp = {0};
  if (len < sizeof(*req) || len != sizeof(*req) + req->key_len) {
    goto cleanup;
  }

  DBGPRINT((void const *)(req + 1), req->key_len, NULL, 0);
#ifdef EXTRAM2_DEBUG
  {
    char buf[256];
    wsprintfA(buf, "[%s:%d] %s: width: %d height: %d", __FILE_NAME__, __LINE__, __func__, req->width, req->height);
    OutputDebugStringA(buf);
  }
#endif

  void *d = kvs_set(g_kvs,
                    (void const *)(req + 1),
                    req->key_len,
                    req->width,
                    req->height,
                    GetTickCount64(),
                    req->width * sizeof(struct pixel) * req->height);
  if (!d) {
    goto cleanup;
  }
  memcpy(d, g_view, req->width * sizeof(struct pixel) * req->height);
  resp.ok = true;
cleanup:
  output(&resp, sizeof(resp), NULL, 0);
}

static void get_str(void *const ptr, size_t const len) {
  struct cmd_get_str_req const *const req = ptr;
  struct cmd_get_str_resp resp = {0};
  if (len < sizeof(*req) || len != sizeof(*req) + req->key_len) {
    goto cleanup;
  }

  struct stored_data *const sd = kvs_get(g_kvs, (void const *)(req + 1), req->key_len);
  if (!sd || sd->width != 0 || sd->height == 0 || sd->p == NULL) {
    goto cleanup;
  }

  DBGPRINT((void const *)(req + 1), req->key_len, sd->p, sd->height);

  sd->used_at = GetTickCount64();
  resp.ok = true;
  resp.val_len = sd->height;
cleanup:
  if (resp.ok) {
    output(&resp, sizeof(resp), sd->p, sd->height);
  } else {
    output(&resp, sizeof(resp), NULL, 0);
  }
}

static void set_str(void *const ptr, size_t const len) {
  struct cmd_set_str_req const *const req = ptr;
  struct cmd_set_str_resp resp = {0};
  if (len < sizeof(*req) || len != sizeof(*req) + req->key_len + req->val_len) {
    goto cleanup;
  }

  uint8_t const *s = (void const *)(req + 1);
  uint8_t const *const key = s;
  void *d = kvs_set(g_kvs, key, req->key_len, 0, req->val_len, GetTickCount64(), req->val_len);
  if (!d) {
    goto cleanup;
  }

  uint8_t const *const val = s + req->key_len;
  memcpy(d, val, req->val_len);

  DBGPRINT(key, req->key_len, val, req->val_len);

  resp.ok = true;
cleanup:
  output(&resp, sizeof(resp), NULL, 0);
}

int main(void) {
  char const *err = NULL;
  void *buf = NULL;

  g_stdin = GetStdHandle(STD_INPUT_HANDLE);
  g_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

  char fmo_name[32];
  GetEnvironmentVariableA("EXTRAM2_FMO", fmo_name, 32);
  g_fmo = OpenFileMappingA(FILE_MAP_WRITE, FALSE, fmo_name);
  if (!g_fmo) {
    err = "failed to open file mapping object";
    goto cleanup;
  }
  g_view = MapViewOfFile(g_fmo, FILE_MAP_WRITE, 0, 0, 0);
  if (!g_view) {
    err = "failed to map view of file";
    goto cleanup;
  }
  uint64_t x = GetTickCount64();
  g_kvs = kvs_init(splitmix64(&x), splitmix64(&x));
  if (!g_kvs) {
    err = "failed to initialize hashmap";
    goto cleanup;
  }

  int32_t cap = 1024;
  int32_t len = 0;
  buf = malloc(cap);
  if (!buf) {
    err = "cannot allocate memory";
    goto cleanup;
  }

  for (;;) {
    DWORD read;
    if (!ReadFile(g_stdin, &len, sizeof(len), &read, NULL) || read != sizeof(len)) {
      err = "cannot read input data size";
      goto cleanup;
    }
    if (!len) {
      break;
    }
    if (len >= 1 * 1024 * 1024) {
      err = "input data too large";
      goto cleanup;
    }
    if (len > cap) {
      free(buf);
      cap = len;
      buf = malloc(cap);
      if (!buf) {
        err = "cannot allocate memory";
        goto cleanup;
      }
    }
    if (!ReadFile(g_stdin, buf, len, &read, NULL) || read != (DWORD)len) {
      err = "cannot read input data";
      goto cleanup;
    }
#ifdef EXTRAM2_DEBUG
    {
      char buf2[256];
      wsprintfA(buf2,
                "[%s:%d] %s cmdid: %d len: %d",
                __FILE_NAME__,
                __LINE__,
                __func__,
                ((struct cmd_unk_req const *)buf)->id,
                len);
      OutputDebugStringA(buf2);
    }
#endif
    switch (((struct cmd_unk_req const *)buf)->id) {
    case CMD_DEL:
      del(buf, len);
      continue;
    case CMD_SET:
      set(buf, len);
      continue;
    case CMD_GET:
      get(buf, len);
      continue;
    case CMD_GET_SIZE:
      get_size(buf, len);
      continue;
    case CMD_SET_STR:
      set_str(buf, len);
      continue;
    case CMD_GET_STR:
      get_str(buf, len);
      continue;
    }
    break;
  }

cleanup:
  if (buf) {
    free(buf);
    buf = NULL;
  }
  if (g_kvs) {
    kvs_destroy(g_kvs);
    g_kvs = NULL;
  }
  if (g_view) {
    UnmapViewOfFile(g_view);
    g_view = NULL;
  }
  if (g_fmo) {
    CloseHandle(g_fmo);
    g_fmo = NULL;
  }
#ifdef EXTRAM2_DEBUG
  if (err) {
    OutputDebugStringA(err);
  }
#endif
  return err ? 1 : 0;
}
