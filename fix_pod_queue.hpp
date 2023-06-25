#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <queue>
#include <thread>

// lock free SPSC for POD
// size must be 2^N
template <typename T>
class FixPodQueue {
 public:
  FixPodQueue(int size);
  ~FixPodQueue();
  bool Initialize();
  bool Write(T *vaule);
  bool Read(T *value);
  bool Empty() { return head_.load() == tail_.load(); }

 private:
  T *pod_queue_;
  int size_;
  int mask_;
  std::atomic<int> head_;  // pop position
  std::atomic<int> tail_;  // push position
};

template <typename T>
FixPodQueue<T>::FixPodQueue(int size) {
  size_ = size;
  mask_ = size - 1;
}

template <typename T>
bool FixPodQueue<T>::Initialize() {
  pod_queue_ = (T *)malloc(size_ * sizeof(T));
  if (pod_queue_ == nullptr) return false;
  head_ = 0;
  tail_ = 0;
  return true;
}

template <typename T>
FixPodQueue<T>::~FixPodQueue() {
  if (size_) free(pod_queue_);
}

template <typename T>
bool FixPodQueue<T>::Write(T *value) {
  if (head_ == ((tail_ + 1) & mask_)) return false;
  memcpy(pod_queue_ + tail_, value, sizeof(T));
  tail_ = (tail_ + 1) & mask_;
  return true;
}

template <typename T>
bool FixPodQueue<T>::Read(T *value) {
  if (head_ == tail_) return false;
  memcpy(value, pod_queue_ + head_, sizeof(T));
  head_ = (head_ + 1) & mask_;
  return true;
}
