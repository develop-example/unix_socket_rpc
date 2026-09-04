#include "rpc/rpc_server.hpp"

#include "rpc/protocol.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

RpcServer::RpcServer(std::string socket_path, std::size_t worker_threads)
    : socket_path_(std::move(socket_path)),
      thread_pool_(std::make_unique<rpc::ThreadPool>(worker_threads)) {

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

  if (thread_pool_) {

    thread_pool_->stop();
  }

  unlink(socket_path_.c_str());
}

void RpcServer::run() {

  running_ = true;

  std::cout << "RPC Server listening on " << socket_path_ << std::endl;

  while (running_) {

    int client_fd = accept(server_fd_, nullptr, nullptr);

    if (client_fd < 0) {

      // stop() 导致 accept 返回
      if (!running_) {
        break;
      }

      // 被信号中断等情况
      continue;
    }

    auto connection = std::make_shared<rpc::ConnectionContext>(client_fd);

    std::thread(&RpcServer::handle_client, this, connection).detach();
  }
}

void RpcServer::handle_client(
    std::shared_ptr<rpc::ConnectionContext> connection) {

  std::cout << "Client connected" << std::endl;

  while (running_ && connection->alive) {

    std::string request_payload;

    // ====================================
    // Receive Request
    // ====================================

    if (!rpc::recv_message(connection->fd, request_payload)) {

      break;
    }

    try {

      // ====================================
      // Parse Request
      // ====================================

      rpc::RpcRequest request =
          rpc::json::parse(request_payload).get<rpc::RpcRequest>();

      // ====================================
      // Dispatch Request
      // ====================================

      thread_pool_->submit(
          [this, connection, request = std::move(request)]() mutable {
            process_request(connection, std::move(request));
          });

    } catch (const std::exception &e) {

      // Request 格式错误
      //
      // 没有可靠 ID 时无法匹配
      // 暂时只记录日志

      std::cerr << "Invalid RPC request: " << e.what() << std::endl;
    }
  }

  // ====================================
  // Connection Closed
  // ====================================

  connection->alive = false;

  shutdown(connection->fd, SHUT_RDWR);

  close(connection->fd);

  std::cout << "Client disconnected" << std::endl;
}

void RpcServer::process_request(
    std::shared_ptr<rpc::ConnectionContext> connection,

    rpc::RpcRequest request) {

  rpc::RpcResponse response;

  response.id = request.id;

  try {

    // ====================================
    // Find RPC Handler
    // ====================================

    auto it = handlers_.find(request.method);

    if (it == handlers_.end()) {

      response.error = rpc::RpcError{
          .code = -32601, .message = "Method not found: " + request.method};

    } else {

      // ====================================
      // Execute Handler
      // ====================================

      response.result = it->second(request.params);
    }

  } catch (const std::exception &e) {

    response.error = rpc::RpcError{.code = -32603, .message = e.what()};
  }

  // ====================================
  // Connection Already Closed?
  // ====================================

  if (!connection->alive) {
    return;
  }

  // ====================================
  // Serialize Response
  // ====================================

  std::string response_payload = rpc::json(response).dump();

  // ====================================
  // Serialize Socket Writes
  // ====================================

  std::lock_guard<std::mutex> lock(connection->send_mutex);

  // 再检查一次
  if (!connection->alive) {
    return;
  }

  if (!rpc::send_message(connection->fd, response_payload)) {

    connection->alive = false;
  }
}

void RpcServer::stop() {

  if (!running_.exchange(false)) {
    return;
  }

  if (server_fd_ >= 0) {

    shutdown(server_fd_, SHUT_RDWR);

    close(server_fd_);

    server_fd_ = -1;
  }
}