#pragma once

#include <stdint.h>

enum command {
  CMD_UNKNOWN,
  CMD_DEL,
  CMD_SET,
  CMD_GET,
  CMD_GET_SIZE,
  CMD_SET_STR,
  CMD_GET_STR,
};

struct cmd_unk_req {
  uint8_t id;
};

struct cmd_del_req {
  uint8_t id;
  uint32_t key_len;
};

struct cmd_del_resp {
  bool ok;
};

struct cmd_set_req {
  uint8_t id;
  uint32_t key_len;
  uint32_t width;
  uint32_t height;
};

struct cmd_set_resp {
  bool ok;
};

struct cmd_get_req {
  uint8_t id;
  uint32_t key_len;
};

struct cmd_get_resp {
  bool ok;
  uint32_t width;
  uint32_t height;
};

struct cmd_get_size_req {
  uint8_t id;
  uint32_t key_len;
};

struct cmd_get_size_resp {
  bool ok;
  uint32_t width;
  uint32_t height;
};

struct cmd_set_str_req {
  uint8_t id;
  uint32_t key_len;
  uint32_t val_len;
};

struct cmd_set_str_resp {
  bool ok;
};

struct cmd_get_str_req {
  uint8_t id;
  uint32_t key_len;
};

struct cmd_get_str_resp {
  bool ok;
  uint32_t val_len;
};
