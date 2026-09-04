#pragma once

#include "rpc/protocol.hpp"
#include "rpc/rpc_message.hpp"
#include "rpc/serialization.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

class RpcClient {
public:
  explicit RpcClient(std::string socket_path);

  template <typename Return, typename... Args>
  Return call(const std::string &method, Args &&...args) {

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd < 0) {
      throw std::runtime_error("Failed to create socket");
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    std::strncpy(addr.sun_path, socket_path_.c_str(),
                 sizeof(addr.sun_path) - 1);

    if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {

      close(fd);

      throw std::runtime_error("Failed to connect RPC server");
    }

    try {

      // 创建 Request
      rpc::RpcRequest request{
          .id = next_request_id_++,
          .method = method,
          .params = rpc::make_params(std::forward<Args>(args)...)};

      // Serialize
      std::string request_payload = rpc::json(request).dump();

      // Send
      if (!rpc::send_message(fd, request_payload)) {

        throw std::runtime_error("Failed to send RPC request");
      }

      // Receive
      std::string response_payload;

      if (!rpc::recv_message(fd, response_payload)) {

        throw std::runtime_error("Failed to receive RPC response");
      }

      // Deserialize
      rpc::RpcResponse response =
          rpc::json::parse(response_payload).get<rpc::RpcResponse>();

      if (response.id != request.id) {
        throw std::runtime_error("RPC response ID mismatch");
      }

      if (response.error) {
        throw std::runtime_error(response.error->message);
      }

      // 返回类型
      if constexpr (std::is_void_v<Return>) {

        return;

      } else {

        return response.result.get<Return>();
      }

    } catch (...) {

      close(fd);
      throw;
    }

    close(fd);
  }

private:
  std::string socket_path_;

  std::atomic<uint64_t> next_request_id_{1};
};