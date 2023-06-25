#include "async_client.h"

#define BATCH 1e5

int cnt = 0;

int main() {
  // AsyncRedisClient clinet("127.0.0.1", 6379);

  std::shared_ptr<AsyncRedisClient> clinet =
      std::make_shared<AsyncRedisClient>("127.0.0.1", 6379);
  if (!clinet->Initialize()) {
    std::cout << "init client error \n";
    exit(0);
  }
  std::cout << "connect ok \n";
  std::vector<std::thread> threads;

  int w_num = 2;
  struct timeval start, end;
  gettimeofday(&start, nullptr);

  for (int i = 0; i < w_num; i++) {
    threads.emplace_back([=]() {
      for (int t = i * BATCH / w_num;
           t < std::min((i + 1) * BATCH / w_num, BATCH); t++) {
        auto f = clinet->SendCommand<CMD::kSET>(std::to_string(t),
                                                std::string("value"));
      }
    });
  }

  // for (int i = 0; i < BATCH; i++) {
  //   auto f =
  //       clinet->SendCommand<CMD::kSET>(std::to_string(i),
  //       std::string("value"));
  // }
  // gettimeofday(&end, nullptr);
  // double elpased = ((end.tv_sec - start.tv_sec) * 1000 +
  //                   (end.tv_usec - start.tv_usec) / 1000);

  // std::cout << "run time(ms): " << elpased
  //           << ", qps: " << BATCH * 1000 / elpased << std::endl;
  for (auto& w : threads) w.join();
  clinet->Stop();
  gettimeofday(&end, nullptr);
  double elpased = ((end.tv_sec - start.tv_sec) * 1000 +
                    (end.tv_usec - start.tv_usec) / 1000);

  std::cout << "consume cnt: " << cnt << ", run time(ms): " << elpased
            << ", qps: " << cnt * 1000 / elpased << std::endl;

  return 0;
}
