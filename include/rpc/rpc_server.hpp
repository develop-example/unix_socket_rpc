#pragma once

#include <functional>
#include <string>
#include <unordered_map>

class RpcServer {
public:
  explicit RpcServer(std::string socket_path);

  ~RpcServer();

  void register_method(const std::string &name,
                       std::function<int(int, int)> handler);

  void run();

private:
  std::string socket_path_;

  int server_fd_ = -1;

  std::unordered_map<std::string, std::function<int(int, int)>> handlers_;
};