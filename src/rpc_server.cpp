#include "rpc/rpc_server.hpp"

#include "rpc/protocol.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

RpcServer::RpcServer(std::string socket_path)
    : socket_path_(std::move(socket_path)) {

  unlink(socket_path_.c_str());

  server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);

  if (server_fd_ < 0) {
    throw std::runtime_error("Failed to create server socket");
  }

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;

  if (socket_path_.size() >= sizeof(addr.sun_path)) {
    close(server_fd_);
    throw std::runtime_error("Unix socket path too long");
  }

  std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(server_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {

    close(server_fd_);

    throw std::runtime_error("Failed to bind socket");
  }

  if (listen(server_fd_, 128) < 0) {

    close(server_fd_);

    throw std::runtime_error("Failed to listen");
  }
}

RpcServer::~RpcServer() {
  stop();

  unlink(socket_path_.c_str());
}

void RpcServer::run() {

  running_ = true;

  std::cout << "RPC Server listening on " << socket_path_ << std::endl;

  while (running_) {

    int client_fd = accept(server_fd_, nullptr, nullptr);

    if (client_fd < 0) {

      if (!running_) {
        break;
      }

      continue;
    }

    // =====================================
    // 每个 Client 一个 Worker Thread
    // =====================================

    std::thread(&RpcServer::handle_client, this, client_fd).detach();
  }
}

void RpcServer::handle_client(int client_fd) {
  std::cout << "Client connected" << std::endl;

  // =====================================
  // Persistent Connection Loop
  // =====================================

  while (running_) {

    std::string request_payload;

    // 等待下一次 RPC Request
    if (!rpc::recv_message(client_fd, request_payload)) {

      // Client disconnect
      break;
    }

    rpc::RpcResponse response;

    try {

      // =====================================
      // JSON -> RPC Request
      // =====================================

      rpc::RpcRequest request =
          rpc::json::parse(request_payload).get<rpc::RpcRequest>();

      response.id = request.id;

      // =====================================
      // Find Handler
      // =====================================

      auto it = handlers_.find(request.method);

      if (it == handlers_.end()) {

        response.error = rpc::RpcError{
            .code = -32601, .message = "Method not found: " + request.method};

      } else {

        // =====================================
        // Execute RPC
        // =====================================

        response.result = it->second(request.params);
      }

    } catch (const std::exception &e) {

      response.error = rpc::RpcError{.code = -32603, .message = e.what()};
    }

    // =====================================
    // Serialize Response
    // =====================================

    std::string response_payload = rpc::json(response).dump();

    // =====================================
    // Send Response
    // =====================================

    if (!rpc::send_message(client_fd, response_payload)) {

      break;
    }
  }

  close(client_fd);

  std::cout << "Client disconnected" << std::endl;
}

void RpcServer::stop() {

  bool expected = true;

  if (running_.compare_exchange_strong(expected, false)) {

    if (server_fd_ >= 0) {

      close(server_fd_);

      server_fd_ = -1;
    }
  }
}