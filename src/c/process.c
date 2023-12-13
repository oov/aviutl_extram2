#include "process.h"

#ifdef __GNUC__
#  ifndef __has_warning
#    define __has_warning(x) 0
#  endif
#  pragma GCC diagnostic push
#  if __has_warning("-Wreserved-identifier")
#    pragma GCC diagnostic ignored "-Wreserved-identifier"
#  endif
#endif // __GNUC__
#include <tinycthread.h>
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif // __GNUC__

#include <stdint.h>
#include <wchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct process {
  HANDLE process;
  thrd_t thread;

  mtx_t mtx;
  cnd_t cnd;
  cnd_t cnd2;
  void *readbuf;
  size_t readbuf_len;

  HANDLE in_w;
  HANDLE out_r;
  HANDLE err_r;
};

static wchar_t *build_environment_strings(wchar_t const *const name, wchar_t const *const value) {
  wchar_t *const envstr = GetEnvironmentStringsW();
  if (!envstr) {
    return NULL;
  }

  int const name_len = wcslen(name);
  int const value_len = wcslen(value);
  int len = name_len + 1 + value_len + 1;
  wchar_t const *src = envstr;
  while (*src) {
    int const l = wcslen(src) + 1;
    len += l;
    src += l;
  }

  wchar_t *newenv = calloc((size_t)(len + 1), sizeof(wchar_t));
  if (!newenv) {
    goto cleanup;
  }

  wchar_t *dest = newenv;
  wcscpy(dest, name);
  dest += name_len;
  *dest++ = L'=';
  len -= name_len + 1;
  wcscpy(dest, value);
  dest += value_len + 1;
  len -= value_len + 1;

  memcpy(dest, envstr, (size_t)(len) * sizeof(wchar_t));
  FreeEnvironmentStringsW(envstr);
  return newenv;
cleanup:
  FreeEnvironmentStringsW(envstr);
  free(newenv);
  return NULL;
}

static wchar_t *get_working_directory(wchar_t const *exe_path) {
  size_t const exe_pathlen = wcslen(exe_path) + 1;
  wchar_t *path = calloc((size_t)exe_pathlen, sizeof(wchar_t));
  if (!path) {
    return NULL;
  }
  int pathlen = 0;
  if (*exe_path == L'"') {
    ++exe_path;
    while (*exe_path != L'\0' && *exe_path != L'"') {
      path[pathlen++] = *exe_path++;
    }
  } else {
    while (*exe_path != L'\0' && *exe_path != L' ') {
      path[pathlen++] = *exe_path++;
    }
  }
  size_t dirlen = (size_t)GetFullPathNameW(path, 0, NULL, NULL);
  if (dirlen == 0) {
    free(path);
    return NULL;
  }
  wchar_t *const dir = calloc((size_t)dirlen, sizeof(wchar_t));
  wchar_t *fn = NULL;
  if (GetFullPathNameW(path, (DWORD)dirlen, dir, &fn) == 0 || fn == NULL) {
    free(dir);
    free(path);
    return NULL;
  }
  *fn = '\0';
  free(path);
  return dir;
}

static bool read(HANDLE h, void *buf, DWORD sz) {
  char *b = buf;
  for (DWORD read; sz > 0; b += read, sz -= read) {
    if (!ReadFile(h, b, sz, &read, NULL)) {
      return false;
    }
  }
  return true;
}

static bool write(HANDLE h, const void *buf, DWORD sz) {
  const char *b = buf;
  for (DWORD written; sz > 0; b += written, sz -= written) {
    if (!WriteFile(h, b, sz, &written, NULL)) {
      return false;
    }
  }
  return true;
}

static int read_worker(void *userdata) {
  struct process *const self = userdata;
  void *buf1 = NULL;
  void *buf2 = NULL;
  size_t buf1_len = 0;
  size_t buf2_len = 0;
  while (1) {
    int32_t sz;
    if (!read(self->out_r, &sz, sizeof(sz))) {
      goto error;
    }
    if ((size_t)sz > buf1_len) {
      buf1 = realloc(buf1, (size_t)sz);
      if (!buf1) {
        goto error;
      }
      buf1_len = (size_t)sz;
    }
    if (sz) {
      if (!read(self->out_r, buf1, (DWORD)sz)) {
        goto error;
      }
    }
    mtx_lock(&self->mtx);
    while (self->readbuf != NULL) {
      cnd_wait(&self->cnd2, &self->mtx);
    }
    self->readbuf = buf1;
    self->readbuf_len = (size_t)sz;
    cnd_signal(&self->cnd);
    mtx_unlock(&self->mtx);
    void *tmp = buf1;
    buf1 = buf2;
    buf2 = tmp;
    size_t tmp_len = buf1_len;
    buf1_len = buf2_len;
    buf2_len = tmp_len;
  }

error:
  mtx_lock(&self->mtx);
  self->readbuf = NULL;
  self->readbuf_len = 0;
  mtx_unlock(&self->mtx);
  if (buf1) {
    free(buf1);
  }
  if (buf2) {
    free(buf2);
  }
  return 1;
}

int process_write(struct process *const self, void const *const buf, size_t const len) {
  int32_t sz = (int32_t)len;
  if (!write(self->in_w, &sz, sizeof(sz))) {
    return 1;
  }
  if (!write(self->in_w, buf, len)) {
    return 3;
  }
  if (!FlushFileBuffers(self->in_w)) {
    return 1;
  }
  return 0;
}

int process_read(struct process *const self,
                 void (*recv)(void *userdata, void const *const ptr, size_t const len),
                 void *userdata) {
  mtx_lock(&self->mtx);
  while (self->readbuf == NULL) {
    cnd_wait(&self->cnd, &self->mtx);
  }
  recv(userdata, self->readbuf, self->readbuf_len);
  self->readbuf = NULL;
  mtx_unlock(&self->mtx);
  return 0;
}

struct process *
process_start(wchar_t const *const exe_path, wchar_t const *const envvar_name, wchar_t const *const envvar_value) {
  HANDLE in_r = INVALID_HANDLE_VALUE;
  HANDLE in_w = INVALID_HANDLE_VALUE;
  HANDLE in_w_tmp = INVALID_HANDLE_VALUE;

  HANDLE out_r = INVALID_HANDLE_VALUE;
  HANDLE out_w = INVALID_HANDLE_VALUE;
  HANDLE out_r_tmp = INVALID_HANDLE_VALUE;

  HANDLE err_r = INVALID_HANDLE_VALUE;
  HANDLE err_w = INVALID_HANDLE_VALUE;
  HANDLE err_r_tmp = INVALID_HANDLE_VALUE;

  wchar_t *env = NULL, *path = NULL, *dir = NULL;

  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), 0, TRUE};
  if (!CreatePipe(&in_r, &in_w_tmp, &sa, 0)) {
    goto cleanup;
  }
  if (!CreatePipe(&out_r_tmp, &out_w, &sa, 0)) {
    goto cleanup;
  }
  if (!CreatePipe(&err_r_tmp, &err_w, &sa, 0)) {
    goto cleanup;
  }

  HANDLE curproc = GetCurrentProcess();
  if (!DuplicateHandle(curproc, in_w_tmp, curproc, &in_w, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
    goto cleanup;
  }
  if (!DuplicateHandle(curproc, out_r_tmp, curproc, &out_r, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
    goto cleanup;
  }
  if (!DuplicateHandle(curproc, err_r_tmp, curproc, &err_r, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
    goto cleanup;
  }

  CloseHandle(in_w_tmp);
  in_w_tmp = INVALID_HANDLE_VALUE;
  CloseHandle(out_r_tmp);
  out_r_tmp = INVALID_HANDLE_VALUE;
  CloseHandle(err_r_tmp);
  err_r_tmp = INVALID_HANDLE_VALUE;

  env = build_environment_strings(envvar_name, envvar_value);
  if (!env) {
    goto cleanup;
  }

  // have to copy this buffer because CreateProcessW may modify path string.
  size_t const pathlen = wcslen(exe_path) + 1;
  path = calloc(pathlen, sizeof(wchar_t));
  if (!path) {
    goto cleanup;
  }
  wsprintfW(path, L"%s", exe_path);

  dir = get_working_directory(exe_path);
  if (!dir) {
    goto cleanup;
  }

  PROCESS_INFORMATION pi = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0};
  STARTUPINFOW si = {0};
  si.cb = sizeof(STARTUPINFOW);
  si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  si.hStdInput = in_r;
  si.hStdOutput = out_w;
  si.hStdError = err_w;
  if (!CreateProcessW(0, path, NULL, NULL, TRUE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, env, dir, &si, &pi)) {
    goto cleanup;
  }
  free(dir);
  dir = NULL;
  free(path);
  path = NULL;
  free(env);
  env = NULL;
  CloseHandle(pi.hThread);
  pi.hThread = INVALID_HANDLE_VALUE;
  CloseHandle(in_r);
  in_r = INVALID_HANDLE_VALUE;
  CloseHandle(err_w);
  err_w = INVALID_HANDLE_VALUE;
  CloseHandle(out_w);
  out_w = INVALID_HANDLE_VALUE;

  struct process *r = calloc(1, sizeof(struct process));
  if (!r) {
    CloseHandle(pi.hProcess);
    pi.hProcess = INVALID_HANDLE_VALUE;
    goto cleanup;
  }
  *r = (struct process){
      .process = pi.hProcess,
      .in_w = in_w,
      .out_r = out_r,
      .err_r = err_r,
  };

  mtx_init(&r->mtx, mtx_plain);
  cnd_init(&r->cnd);
  cnd_init(&r->cnd2);

  if (thrd_create(&r->thread, read_worker, r) != thrd_success) {
    cnd_destroy(&r->cnd2);
    cnd_destroy(&r->cnd);
    mtx_destroy(&r->mtx);
    CloseHandle(pi.hProcess);
    pi.hProcess = INVALID_HANDLE_VALUE;
    free(r);
    goto cleanup;
  }
  return r;

cleanup:
  if (dir) {
    free(dir);
    dir = NULL;
  }
  if (path) {
    free(path);
    path = NULL;
  }
  if (env) {
    free(env);
    env = NULL;
  }

  if (pi.hThread != INVALID_HANDLE_VALUE) {
    CloseHandle(pi.hThread);
    pi.hThread = INVALID_HANDLE_VALUE;
  }
  if (pi.hProcess != INVALID_HANDLE_VALUE) {
    CloseHandle(pi.hProcess);
    pi.hProcess = INVALID_HANDLE_VALUE;
  }

  if (err_r != INVALID_HANDLE_VALUE) {
    CloseHandle(err_r);
    err_r = INVALID_HANDLE_VALUE;
  }
  if (err_w != INVALID_HANDLE_VALUE) {
    CloseHandle(err_w);
    err_w = INVALID_HANDLE_VALUE;
  }
  if (err_r_tmp != INVALID_HANDLE_VALUE) {
    CloseHandle(err_r_tmp);
    err_r_tmp = INVALID_HANDLE_VALUE;
  }
  if (out_r != INVALID_HANDLE_VALUE) {
    CloseHandle(out_r);
    out_r = INVALID_HANDLE_VALUE;
  }
  if (out_w != INVALID_HANDLE_VALUE) {
    CloseHandle(out_w);
    out_w = INVALID_HANDLE_VALUE;
  }
  if (out_r_tmp != INVALID_HANDLE_VALUE) {
    CloseHandle(out_r_tmp);
    out_r_tmp = INVALID_HANDLE_VALUE;
  }
  if (in_r != INVALID_HANDLE_VALUE) {
    CloseHandle(in_r);
    in_r = INVALID_HANDLE_VALUE;
  }
  if (in_w != INVALID_HANDLE_VALUE) {
    CloseHandle(in_w);
    in_w = INVALID_HANDLE_VALUE;
  }
  if (in_w_tmp != INVALID_HANDLE_VALUE) {
    CloseHandle(in_w_tmp);
    in_w_tmp = INVALID_HANDLE_VALUE;
  }
  return NULL;
}

void process_finish(struct process *const self) {
  if (self->in_w != INVALID_HANDLE_VALUE) {
    CloseHandle(self->in_w);
    self->in_w = INVALID_HANDLE_VALUE;
  }
  if (self->out_r != INVALID_HANDLE_VALUE) {
    CloseHandle(self->out_r);
    self->out_r = INVALID_HANDLE_VALUE;
  }
  if (self->err_r != INVALID_HANDLE_VALUE) {
    CloseHandle(self->err_r);
    self->err_r = INVALID_HANDLE_VALUE;
  }
  if (self->process != INVALID_HANDLE_VALUE) {
    CloseHandle(self->process);
    self->process = INVALID_HANDLE_VALUE;
  }

  thrd_join(self->thread, NULL);
  cnd_destroy(&self->cnd2);
  cnd_destroy(&self->cnd);
  mtx_destroy(&self->mtx);
  free(self);
}

void process_close_stderr(struct process *const self) {
  CloseHandle(self->err_r);
  self->err_r = INVALID_HANDLE_VALUE;
}

bool process_isrunning(struct process const *const self) {
  return WaitForSingleObject(self->process, 0) == WAIT_TIMEOUT;
}
