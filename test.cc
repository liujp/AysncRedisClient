#include "async_client.h"

#define BATCH 1e5

int cnt = 0;

int main() {
  AsyncRedisClient client("127.0.0.1", 6379);

  if (!client->Initialize()) {
    std::cout << "init client error \n";
    exit(0);
  }
  std::cout << "connect ok \n";

  struct timeval start, end;
  gettimeofday(&start, nullptr);
  for (int i = 0; i < BATCH; i++) {
    auto f =
        client->SendCommand<CMD::kSET>(std::to_string(i),
        std::string("value"));
  }

  client->Stop();
  gettimeofday(&end, nullptr);
  double elpased = ((end.tv_sec - start.tv_sec) * 1000 +
                    (end.tv_usec - start.tv_usec) / 1000);

  std::cout << "consume cnt: " << cnt << ", run time(ms): " << elpased
            << ", qps: " << cnt * 1000 / elpased << std::endl;

  return 0;
}
