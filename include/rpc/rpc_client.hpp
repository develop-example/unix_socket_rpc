#pragma once

#include "rpc/pending_call.hpp"
#include "rpc/protocol.hpp"
#include "rpc/rpc_message.hpp"
#include "rpc/serialization.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>

class RpcClient {
public:
  explicit RpcClient(std::string socket_path)
      : socket_path_(std::move(socket_path)) {

    connect();

    running_ = true;

    // 专门负责接收 Response
    receive_thread_ = std::thread(&RpcClient::receive_loop, this);
  }

  ~RpcClient() { disconnect(); }

  RpcClient(const RpcClient &) = delete;

  RpcClient &operator=(const RpcClient &) = delete;

  // ========================================
  // Async RPC
  // ========================================

  template <typename Return, typename... Args>
  std::future<Return> call_async(const std::string &method, Args &&...args) {

    if (!running_) {
      throw std::runtime_error("RPC client is disconnected");
    }

    // Generate Request ID
    uint64_t request_id = next_request_id_++;

    // ========================================
    // Create Pending Call
    // ========================================

    auto pending = std::make_shared<rpc::PendingCall<Return>>();

    std::future<Return> future = pending->get_future();

    // ========================================
    // Register Pending Request
    // ========================================

    {
      std::lock_guard<std::mutex> lock(pending_mutex_);

      pending_calls_[request_id] = pending;
    }

    // ========================================
    // Create Request
    // ========================================

    rpc::RpcRequest request{.id = request_id,
                            .method = method,
                            .params =
                                rpc::make_params(std::forward<Args>(args)...)};

    std::string request_payload = rpc::json(request).dump();

    // ========================================
    // Send Request
    // ========================================

    {
      std::lock_guard<std::mutex> lock(send_mutex_);

      if (!rpc::send_message(fd_, request_payload)) {

        // Send failed
        remove_pending(request_id);

        throw std::runtime_error("Failed to send RPC request");
      }
    }

    return future;
  }

  // ========================================
  // Synchronous RPC Wrapper
  // ========================================

  template <typename Return, typename... Args>
  Return call(const std::string &method, Args &&...args) {

    auto future = call_async<Return>(method, std::forward<Args>(args)...);

    if constexpr (std::is_void_v<Return>) {

      future.get();
      return;

    } else {

      return future.get();
    }
  }

  bool connected() const { return running_; }

  void disconnect() {

    bool expected = true;

    if (!running_.compare_exchange_strong(expected, false)) {
      return;
    }

    // ========================================
    // Shutdown Socket
    // ========================================

    if (fd_ >= 0) {

      shutdown(fd_, SHUT_RDWR);

      close(fd_);

      fd_ = -1;
    }

    // ========================================
    // Join Receive Thread
    // ========================================

    if (receive_thread_.joinable()) {
      receive_thread_.join();
    }

    // ========================================
    // Fail all Pending Calls
    // ========================================

    fail_all_pending("RPC client disconnected");
  }

private:
  // ========================================
  // Connect
  // ========================================

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

  // ========================================
  // Receive Thread
  // ========================================

  void receive_loop() {

    while (running_) {

      std::string response_payload;

      if (!rpc::recv_message(fd_, response_payload)) {

        break;
      }

      try {

        // JSON -> RpcResponse
        rpc::RpcResponse response =
            rpc::json::parse(response_payload).get<rpc::RpcResponse>();

        std::shared_ptr<rpc::PendingCallBase> pending;

        // ========================================
        // Find Pending Request
        // ========================================

        {
          std::lock_guard<std::mutex> lock(pending_mutex_);

          auto it = pending_calls_.find(response.id);

          if (it == pending_calls_.end()) {
            continue;
          }

          pending = std::move(it->second);

          pending_calls_.erase(it);
        }

        // ========================================
        // Complete Promise
        // ========================================

        pending->set_response(response);

      } catch (const std::exception &) {

        // 收到非法 Response
        // 当前版本简单忽略
      }
    }

    // Receive loop ended
    running_ = false;

    fail_all_pending("RPC connection closed");
  }

  // ========================================
  // Remove Pending
  // ========================================

  void remove_pending(uint64_t request_id) {

    std::lock_guard<std::mutex> lock(pending_mutex_);

    pending_calls_.erase(request_id);
  }

  // ========================================
  // Fail All Pending RPC
  // ========================================

  void fail_all_pending(const std::string &message) {

    std::unordered_map<uint64_t, std::shared_ptr<rpc::PendingCallBase>> pending;

    {
      std::lock_guard<std::mutex> lock(pending_mutex_);

      pending.swap(pending_calls_);
    }

    auto exception = std::make_exception_ptr(std::runtime_error(message));

    for (auto &[id, call] : pending) {

      call->set_exception(exception);
    }
  }

private:
  std::string socket_path_;

  int fd_ = -1;

  std::atomic<bool> running_{false};

  std::atomic<uint64_t> next_request_id_{1};

  // Serialize send_message()
  std::mutex send_mutex_;

  // Protect pending_calls_
  std::mutex pending_mutex_;

  // Dedicated Response Thread
  std::thread receive_thread_;

  // Request ID -> Promise
  std::unordered_map<uint64_t, std::shared_ptr<rpc::PendingCallBase>>
      pending_calls_;
};