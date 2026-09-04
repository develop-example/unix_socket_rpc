#include "rpc/rpc_server.hpp"

int main() {

  RpcServer server("/tmp/demo_rpc.sock");

  server.register_method("add", [](const rpc::json &params) -> rpc::json {
    int a = params[0].get<int>();

    int b = params[1].get<int>();

    return a + b;
  });

  server.register_method("multiply", [](const rpc::json &params) -> rpc::json {
    int a = params[0].get<int>();

    int b = params[1].get<int>();

    return a * b;
  });

  server.run();

  return 0;
}