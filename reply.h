#pragma once

#include <hiredis/hiredis.h>

class RedisReplyView {
 public:
  RedisReplyView(redisReply* r) : reply_(r) {}

 private:
  redisReply* reply_;
};
