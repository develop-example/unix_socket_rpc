#include "rpc/rpc_client.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <string>

RpcClient::RpcClient(std::string socket_path)
    : socket_path_(std::move(socket_path)) {}

int RpcClient::call(const std::string &method, int a, int b) {
  // 1. 创建 Unix Socket
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (fd < 0) {
    throw std::runtime_error("Failed to create socket");
  }

  // 2. 配置 Server 地址
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;

  std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

  // 3. 连接 Server
  if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {

    close(fd);

    throw std::runtime_error("Failed to connect to RPC server");
  }

  // 4. 序列化 RPC 请求
  //
  // add 10 20
  //
  std::string request =
      method + " " + std::to_string(a) + " " + std::to_string(b);

  // 5. 发送请求
  ssize_t sent = write(fd, request.data(), request.size());

  if (sent < 0) {
    close(fd);
    throw std::runtime_error("Failed to send request");
  }

  // 6. 接收响应
  char buffer[1024] = {};

  ssize_t n = read(fd, buffer, sizeof(buffer) - 1);

  close(fd);

  if (n <= 0) {
    throw std::runtime_error("Failed to receive response");
  }

  // 7. 反序列化响应
  return std::stoi(std::string(buffer, n));
}