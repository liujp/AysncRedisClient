#pragma once
#include <assert.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <hiredis/hiredis.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <thread>

#include "command.h"
#include "fix_pod_queue.hpp"

extern int cnt;

template <CMD cmd>
void HandleCallBack(void* reply, void* closure);

template <>
void HandleCallBack<CMD::kGET>(void* r, void* pm) {
  redisReply* reply = static_cast<redisReply*>(r);
  auto* p = static_cast<std::promise<std::string>*>(pm);
  if (reply->type == REDIS_ERR) {
    p->set_value("error");
  }
  if (reply->type != REDIS_REPLY_STRING) {
    p->set_value("type error");
  } else {
    p->set_value(std::string(reply->str, reply->len));
  }
  delete p;
}

template <>
void HandleCallBack<CMD::kSET>(void* r, void* pm) {
  redisReply* reply = static_cast<redisReply*>(r);
  auto* p = static_cast<std::promise<bool>*>(pm);
  if (reply->type == REDIS_ERR) {
    p->set_value(false);
  }
  if (reply->type != REDIS_REPLY_STATUS) {
    p->set_value(false);
  } else {
    p->set_value(true);
  }
  delete p;
  cnt++;
}

class AsyncRedisClient {
 public:
  AsyncRedisClient(const std::string ip, int port) : ip_(ip), port_(port) {}

  bool Initialize();

  // this using client calss, so id client close, wait to handle all the closure
  static void ReadEventHandler(evutil_socket_t fd, short event, void* arg) {
    AsyncRedisClient* client = static_cast<AsyncRedisClient*>(arg);
    client->HandleClosure();
  }

  template <CMD cmd, typename... Args>
  int WriteCommandImp(Args&&... args);

  template <CMD cmd, typename... Args>
  int WriteCommand(Args&&... args) {
    return WriteCommandImp<cmd>(std::forward<Args>(args)...);
  }

  template <CMD cmd, typename... Args>
  auto SendCommand(Args&&... args)
      -> std::future<typename CmdType<cmd>::v_type> {
    std::unique_lock<std::mutex> lk(wl_);
    std::promise<typename CmdType<cmd>::v_type>* p =
        new std::promise<typename CmdType<cmd>::v_type>();

    auto f = p->get_future();

    if (WriteCommand<cmd>(std::forward<Args>(args)...) == REDIS_ERR) {
      std::cout << "write error \n";
      return f;
    }

    Closure e{&HandleCallBack<cmd>, p};
    while (!(que_->Write(&e)))
      ;

    int done = 0;
    do {
      auto ret = redisBufferWrite(c_, &done);
    } while (!done);

    return f;
  }

  void HandleClosure() {
    if (redisBufferRead(c_) == REDIS_ERR) {
      std::cout << "read redis error \n";
      return;
    }

    void* reply = nullptr;
    int status;
    while ((status = redisGetReply(c_, &reply)) == REDIS_OK) {
      if (reply == nullptr) {
        // stop is called and
        break;
      }
      Closure e;
      while (!que_->Read(&e))
        ;
      // sync call
      //   CmdCallBackHandle(reply, &e);
      e.fn(reply, e.data);
      c_->reader->fn->freeObject(reply);
    }  // status is not ok return error
  }

  // forbiden to write and wait queue to empty
  void Stop() {
    while (!(que_->Empty()))
      ;
    std::cout << "queue empty \n";
    event_del(event_);
  }

  ~AsyncRedisClient() {
    if (w_.joinable()) w_.join();
    if (!c_) redisFree(c_);
    event_free(event_);
    event_base_free(base_);
  }

 private:
  std::string ip_;
  int port_;
  redisContext* c_;
  //   std::unique_ptr<boost::lockfree::spsc_queue<Closure>> que_;
  std::unique_ptr<FixPodQueue<Closure>> que_;
  std::thread w_;
  event_base* base_;
  event* event_;
  std::mutex wl_;
  bool stop_;
};

// left ref parameters
template <>
int AsyncRedisClient::WriteCommandImp<CMD::kGET>(const std::string& key) {
  return redisAppendCommand(this->c_, "GET %b", key.c_str(), key.size());
}

template <>
int AsyncRedisClient::WriteCommandImp<CMD::kSET>(const std::string& key,
                                                 const std::string& value) {
  return redisAppendCommand(this->c_, "SET %b %b", key.c_str(), key.size(),
                            value.c_str(), value.size());
}

template <>
int AsyncRedisClient::WriteCommandImp<CMD::kDEL>(const std::string& key) {
  return redisAppendCommand(this->c_, "DEL %b", key.c_str(), key.size());
}

template <>
int AsyncRedisClient::WriteCommandImp<CMD::kTTL>(const std::string& key,
                                                 const int64_t& ttl) {
  return redisAppendCommand(this->c_, "EXPIRE %b %ld", key.c_str(), key.size(),
                            ttl);
}

// move parameters
template <>
int AsyncRedisClient::WriteCommandImp<CMD::kGET>(std::string&& key) {
  return redisAppendCommand(this->c_, "GET %b", key.c_str(), key.size());
}

template <>
int AsyncRedisClient::WriteCommandImp<CMD::kSET>(std::string&& key,
                                                 std::string&& value) {
  return redisAppendCommand(this->c_, "SET %b %b", key.c_str(), key.size(),
                            value.c_str(), value.size());
}

template <>
int AsyncRedisClient::WriteCommandImp<CMD::kDEL>(std::string&& key) {
  return redisAppendCommand(this->c_, "DEL %b", key.c_str(), key.size());
}

template <>
int AsyncRedisClient::WriteCommandImp<CMD::kTTL>(std::string&& key,
                                                 int64_t&& ttl) {
  return redisAppendCommand(this->c_, "EXPIRE %b %ld", key.c_str(), key.size(),
                            ttl);
}
