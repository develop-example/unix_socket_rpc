#pragma once

#include "rpc/protocol.hpp"
#include "rpc/rpc_message.hpp"
#include "rpc/serialization.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

class RpcClient {
public:
  explicit RpcClient(std::string socket_path)
      : socket_path_(std::move(socket_path)) {

    connect();
  }

  ~RpcClient() { disconnect(); }

  RpcClient(const RpcClient &) = delete;
  RpcClient &operator=(const RpcClient &) = delete;

  template <typename Return, typename... Args>
  Return call(const std::string &method, Args &&...args) {
    // 当前版本：
    // 一个连接同一时刻只允许一个 Request/Response
    //
    // 防止多个线程同时 send / recv 导致响应错乱
    std::lock_guard<std::mutex> lock(mutex_);

    if (fd_ < 0) {
      throw std::runtime_error("RPC client is disconnected");
    }

    rpc::RpcRequest request{.id = next_request_id_++,
                            .method = method,
                            .params =
                                rpc::make_params(std::forward<Args>(args)...)};

    std::string request_payload = rpc::json(request).dump();

    // Send Request
    if (!rpc::send_message(fd_, request_payload)) {
      throw std::runtime_error("Failed to send RPC request");
    }

    // Receive Response
    std::string response_payload;

    if (!rpc::recv_message(fd_, response_payload)) {
      throw std::runtime_error("Failed to receive RPC response");
    }

    // Deserialize
    rpc::RpcResponse response =
        rpc::json::parse(response_payload).get<rpc::RpcResponse>();

    // 校验 Request ID
    if (response.id != request.id) {
      throw std::runtime_error("RPC response ID mismatch");
    }

    if (response.error) {
      throw std::runtime_error(response.error->message);
    }

    if constexpr (std::is_void_v<Return>) {
      return;
    } else {
      return response.result.get<Return>();
    }
  }

  bool connected() const { return fd_ >= 0; }

  void disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (fd_ >= 0) {
      close(fd_);
      fd_ = -1;
    }
  }

private:
  void connect() {

    fd_ = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd_ < 0) {
      throw std::runtime_error("Failed to create Unix socket");
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    if (socket_path_.size() >= sizeof(addr.sun_path)) {
      close(fd_);
      fd_ = -1;

      throw std::runtime_error("Unix socket path too long");
    }

    std::strncpy(addr.sun_path, socket_path_.c_str(),
                 sizeof(addr.sun_path) - 1);

    if (::connect(fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {

      close(fd_);
      fd_ = -1;

      throw std::runtime_error("Failed to connect RPC server");
    }
  }

private:
  std::string socket_path_;

  int fd_ = -1;

  std::atomic<uint64_t> next_request_id_{1};

  // 当前同步 RPC 模型的核心锁
  mutable std::mutex mutex_;
};