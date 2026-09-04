#include "rpc/rpc_client.hpp"

#include <iostream>
#include <string>

int main() {

  RpcClient client("/tmp/demo_rpc.sock");

  // int
  int sum = client.call<int>("add", 10, 20);

  std::cout << "add: " << sum << std::endl;

  // double
  double result = client.call<double>("divide", 10.0, 3.0);

  std::cout << "divide: " << result << std::endl;

  // string
  std::string hello = client.call<std::string>("hello", "FWG");

  std::cout << hello << std::endl;

  // void
  client.call<void>("log", "Hello RPC");

  return 0;
}