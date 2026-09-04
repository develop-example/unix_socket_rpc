#include "rpc/rpc_client.hpp"
#include "rpc/protocol.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <string>

RpcClient::RpcClient(std::string socket_path)
    : socket_path_(std::move(socket_path)) {}

int RpcClient::call(const std::string &method, int a, int b) {
  // 创建 Unix Socket
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (fd < 0) {
    throw std::runtime_error("Failed to create socket");
  }

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;

  std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

  // 连接 Server
  if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {

    close(fd);

    throw std::runtime_error("Failed to connect RPC server");
  }

  // 构造 RPC Request
  std::string request =
      method + " " + std::to_string(a) + " " + std::to_string(b);

  // 使用协议层发送
  if (!rpc::send_message(fd, request)) {
    close(fd);

    throw std::runtime_error("Failed to send RPC request");
  }

  // 使用协议层接收
  std::string response;

  if (!rpc::recv_message(fd, response)) {
    close(fd);

    throw std::runtime_error("Failed to receive RPC response");
  }

  close(fd);

  // RPC Error
  if (response.rfind("ERROR", 0) == 0) {
    throw std::runtime_error(response);
  }

  return std::stoi(response);
}