#include "rpc/rpc_server.hpp"

#include "rpc/protocol.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
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

void RpcServer::run() {

  std::cout << "RPC Server listening on " << socket_path_ << std::endl;

  while (true) {

    int client_fd = accept(server_fd_, nullptr, nullptr);

    if (client_fd < 0) {
      continue;
    }

    // ============================
    // Receive Message
    // ============================

    std::string request_payload;

    if (!rpc::recv_message(client_fd, request_payload)) {

      close(client_fd);
      continue;
    }

    rpc::RpcResponse response;

    try {

      // ============================
      // JSON -> RPC Request
      // ============================

      rpc::json request_json = rpc::json::parse(request_payload);

      rpc::RpcRequest request = request_json.get<rpc::RpcRequest>();

      response.id = request.id;

      // ============================
      // Find Method
      // ============================

      auto it = handlers_.find(request.method);

      if (it == handlers_.end()) {

        response.error = rpc::RpcError{
            .code = -32601, .message = "Method not found: " + request.method};

      } else {

        // ============================
        // Execute Handler
        // ============================

        response.result = it->second(request.params);
      }

    } catch (const std::exception &e) {

      response.error = rpc::RpcError{.code = -32603, .message = e.what()};
    }

    // ============================
    // Response -> JSON
    // ============================

    rpc::json response_json = response;

    std::string response_payload = response_json.dump();

    // ============================
    // Send Response
    // ============================

    rpc::send_message(client_fd, response_payload);

    close(client_fd);
  }
}