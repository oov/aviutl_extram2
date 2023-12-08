#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#include "kvs.h"

// #define EXTRAM2_DEBUG

static struct kvs *g_kvs = NULL;

static void output(const char *const fmt, ...) {
  char buf[1024] = {0};
  va_list p;
  va_start(p, fmt);
  const int32_t len = vsprintf(buf, fmt, p);
  va_end(p);
  if (fwrite(&len, sizeof(len), 1, stdout) != 1) {
#ifdef EXTRAM2_DEBUG
    OutputDebugStringA("cannot write output data size");
#endif
    return;
  }
  if (len) {
    if (fwrite(buf, 1, len, stdout) != (size_t)len) {
#ifdef EXTRAM2_DEBUG
      OutputDebugStringA("cannot write output data");
#endif
      return;
    }
  }
  fflush(stdout);
}

static const char *find_next_token(const char *const s, const size_t slen, const char sep) {
  for (size_t i = 0; i < slen; ++i) {
    if (s[i] == sep) {
      return s + i + 1;
    }
  }
  return NULL;
}

struct share_mem_header {
  uint32_t header_size;
  uint32_t body_size;
  uint32_t version;
  uint32_t width;
  uint32_t height;
};

struct pixel {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;
};

struct fmo {
  HANDLE fmo;
  struct share_mem_header *h;
  struct pixel *p;
};

static bool open_fmo(const bool write, struct fmo *const r) {
  char fmo_name[32];
  GetEnvironmentVariableA("BRIDGE_FMO", fmo_name, 32);
  HANDLE fmo = OpenFileMappingA(write ? FILE_MAP_WRITE : FILE_MAP_READ, FALSE, fmo_name);
  if (!fmo) {
    return false;
  }
  void *view = MapViewOfFile(fmo, write ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);
  if (!view) {
    CloseHandle(fmo);
    return false;
  }
  r->fmo = fmo;
  r->h = view;
  r->p = (struct pixel *)((char *)(view) + r->h->header_size);
  return true;
}

static void close_fmo(struct fmo *const fmo) {
  if (fmo->h) {
    UnmapViewOfFile((LPCVOID)fmo->h);
    fmo->h = NULL;
  }
  if (fmo->fmo) {
    CloseHandle(fmo->fmo);
    fmo->fmo = NULL;
  }
}

static void del(char *params, const size_t params_len) {
  kvs_delete(g_kvs, params, params_len);
  output("1");
}

static void get_size(char *params, const size_t params_len) {
  const struct stored_data *const sd = kvs_get(g_kvs, params, params_len);
  if (!sd || sd->width == 0 || sd->height == 0 || sd->p == NULL) {
    output("0");
    return;
  }
  output("1%d %d", sd->width, sd->height);
}

static void get(char *params, const size_t params_len) {
  struct stored_data *sd = kvs_get(g_kvs, params, params_len);
  if (!sd || sd->width == 0 || sd->height == 0 || sd->p == NULL) {
    output("0");
    return;
  }
  struct fmo fmo = {0};
  if (!open_fmo(true, &fmo)) {
    output("0");
    return;
  }

  const int32_t srcw = sd->width, srch = sd->height;
  const struct pixel *s = sd->p;

  const int32_t dstw = fmo.h->width, dsth = fmo.h->height;
  struct pixel *d = fmo.p;

  if (srcw == dstw && srch == dsth) {
    memcpy(d, s, srcw * sizeof(struct pixel) * srch);
  } else {
    const int32_t w = min(srcw, dstw) * sizeof(struct pixel), h = min(srch, dsth);
    for (int32_t y = 0; y < h; ++y) {
      memcpy(d, s, w);
      s += srcw;
      d += dstw;
    }
  }
  close_fmo(&fmo);
  sd->used_at = GetTickCount64();
  output("1");
}

static void set(char *params, const size_t params_len) {
  struct fmo fmo = {0};
  if (!open_fmo(false, &fmo)) {
    output("0");
    return;
  }
  uint32_t const width = fmo.h->width;
  uint32_t const height = fmo.h->height;
  void *ptr =
      kvs_set(g_kvs, params, params_len, width, height, GetTickCount64(), width * sizeof(struct pixel) * height);
  if (ptr) {
    memcpy(ptr, fmo.p, width * sizeof(struct pixel) * height);
  }
  close_fmo(&fmo);
  if (!ptr) {
    output("0");
    return;
  }
  output("1%d %d", width, height);
}

static void get_str(char *params, const size_t params_len) {
  struct stored_data *const sd = kvs_get(g_kvs, params, params_len);
  if (!sd || sd->width != 0 || sd->height == 0 || sd->p == NULL) {
    output("0");
    return;
  }
  sd->used_at = GetTickCount64();
  int32_t len = sd->height + 1;
  if (fwrite(&len, sizeof(len), 1, stdout) != 1) {
#ifdef EXTRAM2_DEBUG
    OutputDebugStringA("cannot write output data size");
#endif
    return;
  }
  if (fwrite("1", 1, 1, stdout) != 1) {
#ifdef EXTRAM2_DEBUG
    OutputDebugStringA("cannot write reply");
#endif
    return;
  }
  if (sd->height > 0) {
    if (fwrite(sd->p, 1, sd->height, stdout) != (size_t)sd->height) {
#ifdef EXTRAM2_DEBUG
      OutputDebugStringA("cannot write output data");
#endif
      return;
    }
  }
  fflush(stdout);
}

static void set_str(char *params, const size_t params_len) {
  const char *key = params;
  const char *value = find_next_token(params, params_len, '\0');
  if (!value) {
    output("0");
    return;
  }
  size_t const key_len = value - key - 1;
  size_t const value_len = params_len - key_len - 1;
  void *ptr = kvs_set(g_kvs, key, key_len, 0, value_len, GetTickCount64(), value_len);
  if (ptr) {
    memcpy(ptr, value, value_len);
  }
  output(ptr ? "1" : "0");
}

int main(void) {
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);

  g_kvs = kvs_init(GetTickCount64());
  if (!g_kvs) {
#ifdef EXTRAM2_DEBUG
    OutputDebugStringA("failed to initialize hashmap");
#endif
    return 1;
  }

  int32_t cap = 1024;
  int32_t len = 0;
  char *buf = malloc(cap);
  if (!buf) {
#ifdef EXTRAM2_DEBUG
    OutputDebugStringA("cannot allocate memory");
#endif
    return 1;
  }
  for (;;) {
    if (fread(&len, sizeof(len), 1, stdin) != 1) {
#ifdef EXTRAM2_DEBUG
      OutputDebugStringA("cannot read input data size");
#endif
      return 1;
    }
    if (len >= 1 * 1024 * 1024) {
#ifdef EXTRAM2_DEBUG
      OutputDebugStringA("input data too large");
#endif
      return 1;
    }
    if (len > cap) {
      free(buf);
      cap = len;
      buf = malloc(cap);
      if (!buf) {
#ifdef EXTRAM2_DEBUG
        OutputDebugStringA("cannot allocate memory");
#endif
        return 1;
      }
    }
    if (fread(buf, 1, len, stdin) != (size_t)len) {
#ifdef EXTRAM2_DEBUG
      OutputDebugStringA("cannot read input data");
#endif
      return 1;
    }

    if (len >= 9 && memcmp("get_size\0", buf, 9) == 0) {
      get_size(buf + 9, len - 9);
      continue;
    } else if (len >= 4 && memcmp("get\0", buf, 4) == 0) {
      get(buf + 4, len - 4);
      continue;
    } else if (len >= 4 && memcmp("set\0", buf, 4) == 0) {
      set(buf + 4, len - 4);
      continue;
    } else if (len >= 8 && memcmp("get_str\0", buf, 8) == 0) {
      get_str(buf + 8, len - 8);
      continue;
    } else if (len >= 8 && memcmp("set_str\0", buf, 8) == 0) {
      set_str(buf + 8, len - 8);
      continue;
    } else if (len >= 4 && memcmp("del\0", buf, 4) == 0) {
      del(buf + 4, len - 4);
      continue;
    }
    output("");
  }

  kvs_destroy(g_kvs);
  g_kvs = NULL;
  return 0;
}
