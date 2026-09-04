#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace rpc {

class ThreadPool {
public:
  explicit ThreadPool(
      std::size_t thread_count = std::thread::hardware_concurrency()) {

    if (thread_count == 0) {
      thread_count = 1;
    }

    workers_.reserve(thread_count);

    for (std::size_t i = 0; i < thread_count; ++i) {

      workers_.emplace_back([this] { worker_loop(); });
    }
  }

  ~ThreadPool() { stop(); }

  ThreadPool(const ThreadPool &) = delete;

  ThreadPool &operator=(const ThreadPool &) = delete;

  // ========================================
  // Submit Task
  // ========================================

  template <typename Func, typename... Args>
  auto submit(Func &&func, Args &&...args)
      -> std::future<std::invoke_result_t<Func, Args...>> {

    using ReturnType = std::invoke_result_t<Func, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(

        std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

    std::future<ReturnType> future = task->get_future();

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);

      if (stopping_) {

        throw std::runtime_error("ThreadPool is stopped");
      }

      tasks_.emplace([task] { (*task)(); });
    }

    condition_.notify_one();

    return future;
  }

  // ========================================
  // Stop
  // ========================================

  void stop() {

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);

      if (stopping_) {
        return;
      }

      stopping_ = true;
    }

    condition_.notify_all();

    for (auto &worker : workers_) {

      if (worker.joinable()) {

        worker.join();
      }
    }

    workers_.clear();
  }

  std::size_t size() const { return workers_.size(); }

private:
  void worker_loop() {

    while (true) {

      std::function<void()> task;

      {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        condition_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });

        if (stopping_ && tasks_.empty()) {

          return;
        }

        task = std::move(tasks_.front());

        tasks_.pop();
      }

      try {

        task();

      } catch (...) {

        // 防止异常导致 Worker Thread 退出
      }
    }
  }

private:
  std::vector<std::thread> workers_;

  std::queue<std::function<void()>> tasks_;

  mutable std::mutex queue_mutex_;

  std::condition_variable condition_;

  bool stopping_ = false;
};

} // namespace rpc