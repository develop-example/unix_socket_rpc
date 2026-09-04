#include "rpc/rpc_server.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {

  RpcServer server("/tmp/demo_rpc.sock");

  server.register_method("slow_add", [](int a, int b) {
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "slow_add finished" << std::endl;

    return a + b;
  });

  server.register_method("fast_add", [](int a, int b) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "fast_add finished" << std::endl;

    return a + b;
  });

  server.run();

  return 0;
}