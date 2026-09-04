#include "rpc/rpc_client.hpp"

#include <iostream>

int main() {

  RpcClient client("/tmp/demo_rpc.sock");

  int result = client.call("add", 10, 20);

  std::cout << "Result: " << result << std::endl;

  return 0;
}