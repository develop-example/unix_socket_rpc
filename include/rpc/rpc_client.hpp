#pragma once

#include <string>

class RpcClient {
public:
  explicit RpcClient(std::string socket_path);

  int call(const std::string &method, int a, int b);

private:
  std::string socket_path_;
};