#include "rpc/rpc_server.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

RpcServer::RpcServer(std::string socket_path)
    : socket_path_(std::move(socket_path)) {

  // 删除旧 socket 文件
  unlink(socket_path_.c_str());

  // 创建 Socket
  server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);

  if (server_fd_ < 0) {
    throw std::runtime_error("Failed to create server socket");
  }

  // 地址
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;

  std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

  // bind
  if (bind(server_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {

    close(server_fd_);

    throw std::runtime_error("Failed to bind socket");
  }

  // listen
  if (listen(server_fd_, 10) < 0) {

    close(server_fd_);

    throw std::runtime_error("Failed to listen");
  }
}

RpcServer::~RpcServer() {

  if (server_fd_ >= 0) {
    close(server_fd_);
  }

  unlink(socket_path_.c_str());
}

void RpcServer::register_method(const std::string &name,
                                std::function<int(int, int)> handler) {
  handlers_[name] = std::move(handler);
}

void RpcServer::run() {

  std::cout << "RPC Server listening on " << socket_path_ << std::endl;

  while (true) {

    // 1. 等待 Client
    int client_fd = accept(server_fd_, nullptr, nullptr);

    if (client_fd < 0) {
      continue;
    }

    // 2. 接收请求
    char buffer[1024] = {};

    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);

    if (n <= 0) {
      close(client_fd);
      continue;
    }

    // 3. 解析请求
    //
    // add 10 20
    //
    std::istringstream iss(std::string(buffer, n));

    std::string method;
    int a, b;

    iss >> method >> a >> b;

    std::string response;

    // 4. 查找 RPC Handler
    auto it = handlers_.find(method);

    if (it == handlers_.end()) {

      response = "ERROR unknown method";

    } else {

      // 5. 调用真实函数
      int result = it->second(a, b);

      // 6. 序列化返回结果
      response = std::to_string(result);
    }

    // 7. 返回响应
    write(client_fd, response.data(), response.size());

    close(client_fd);
  }
}