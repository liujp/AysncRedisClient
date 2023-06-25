#include "async_client.h"

bool AsyncRedisClient::Initialize() {
  // connect socket using no block method
  c_ = redisConnectNonBlock(ip_.c_str(), port_);
  if (nullptr == c_ || c_->err) {
    if (c_) printf("Error: %s, %d\n", c_->errstr, c_->err);
    return false;
  }

  que_ = std::unique_ptr<FixPodQueue<Closure>>(
      new FixPodQueue<Closure>((1 << 10)));
  que_->Initialize();

  evthread_use_pthreads();
  base_ = event_base_new();

  event_ = event_new(base_, c_->fd, EV_READ | EV_PERSIST, ReadEventHandler,
                     (void*)this);
  event_add(event_, nullptr);

  w_ = std::thread([this]() {
    std::cout << "Libevent loop start... \n";
    int ret = event_base_dispatch(this->base_);
    std::cout << "Libevent stop for: "
              << (ret == 1 ? "no pending or active" : "error") << std::endl;
  });

  usleep(10000);
  return true;
}