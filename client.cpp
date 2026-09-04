#include "rpc/rpc_client.hpp"

#include <iostream>
#include <thread>
#include <vector>

void worker(int id) {

  RpcClient client("/tmp/demo_rpc.sock");

  for (int i = 0; i < 10; ++i) {

    int result = client.call<int>("add", id, i);
    printf("Client %d: add(%d, %d) = %d\n", id, id, i, result);
    // std::cout << "Client " << id << ": " << result << std::endl;
  }
}

int main() {

  std::vector<std::thread> threads;

  for (int i = 0; i < 5; ++i) {
    threads.emplace_back(worker, i);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  return 0;
}