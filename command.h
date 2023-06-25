#pragma once

// for cache align
enum CMD : int { kGET = 0, kSET, kDEL, kTTL };

typedef void (*pvCallBack)(void* reply, void* closure);

struct Closure {
  pvCallBack fn;
  void* data;
};

template <CMD cmd>
struct CmdType;

template <>
struct CmdType<kGET> {
  using v_type = std::string;
};

template <>
struct CmdType<kSET> {
  using v_type = bool;
};

template <>
struct CmdType<kDEL> {
  using v_type = long;
};

template <>
struct CmdType<kTTL> {
  using v_type = long;
};