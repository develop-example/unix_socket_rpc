#include "rpc/rpc_server.hpp"
#include "rpc/protocol.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

RpcServer::RpcServer(std::string socket_path)
    : socket_path_(std::move(socket_path)) {

  unlink(socket_path_.c_str());

  server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);

  if (server_fd_ < 0) {
    throw std::runtime_error("Failed to create server socket");
  }

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;

  std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(server_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {

    close(server_fd_);

    throw std::runtime_error("Failed to bind socket");
  }

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

    int client_fd = accept(server_fd_, nullptr, nullptr);

    if (client_fd < 0) {
      continue;
    }

    // ========================
    // Protocol Layer
    // ========================

    std::string request;

    if (!rpc::recv_message(client_fd, request)) {

      close(client_fd);
      continue;
    }

    // ========================
    // RPC Layer
    // ========================

    std::istringstream iss(request);

    std::string method;
    int a, b;

    iss >> method >> a >> b;

    std::string response;

    auto it = handlers_.find(method);

    if (it == handlers_.end()) {

      response = "ERROR unknown method";

    } else {

      int result = it->second(a, b);

      response = std::to_string(result);
    }

    // ========================
    // Protocol Layer
    // ========================

    rpc::send_message(client_fd, response);

    close(client_fd);
  }
}