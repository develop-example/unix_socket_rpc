#include "rpc/rpc_server.hpp"

#include <iostream>
#include <string>

int main() {

  RpcServer server("/tmp/demo_rpc.sock");

  // int(int, int)
  server.register_method("add", [](int a, int b) { return a + b; });

  // double(double, double)
  server.register_method("divide", [](double a, double b) {
    if (b == 0.0) {
      throw std::runtime_error("Division by zero");
    }

    return a / b;
  });

  // std::string(std::string)
  server.register_method("hello",
                         [](std::string name) { return "Hello, " + name; });

  // void(std::string)
  server.register_method("log", [](std::string message) {
    std::cout << "[LOG] " << message << std::endl;
  });

  server.run();

  return 0;
}