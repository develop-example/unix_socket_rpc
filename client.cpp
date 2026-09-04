#include "rpc/rpc_client.hpp"

#include <chrono>
#include <iostream>

int main() {

  RpcClient client("/tmp/demo_rpc.sock");

  auto start = std::chrono::steady_clock::now();

  // ====================================
  // Send Two Requests Immediately
  // ====================================

  auto slow_future = client.call_async<int>("slow_add", 10, 20);

  auto fast_future = client.call_async<int>("fast_add", 30, 40);

  // ====================================
  // Wait Fast First
  // ====================================

  int fast_result = fast_future.get();

  auto fast_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);

  std::cout << "Fast Result: " << fast_result << " (" << fast_time.count()
            << " ms)" << std::endl;

  // ====================================
  // Wait Slow
  // ====================================

  int slow_result = slow_future.get();

  auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);

  std::cout << "Slow Result: " << slow_result << " (" << total_time.count()
            << " ms)" << std::endl;

  return 0;
}