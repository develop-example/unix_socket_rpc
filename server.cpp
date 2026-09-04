#include "rpc/rpc_server.hpp"

int main() {

  RpcServer server("/tmp/demo_rpc.sock");

  // 注册 add RPC
  server.register_method("add", [](int a, int b) { return a + b; });

  // 注册 multiply RPC
  server.register_method("multiply", [](int a, int b) { return a * b; });

  server.run();

  return 0;
}