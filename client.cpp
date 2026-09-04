#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

constexpr const char *SOCKET_PATH = "/tmp/demo_rpc.sock";

int main() {
  // 创建 Socket
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (fd < 0) {
    perror("socket");
    return 1;
  }

  // Server 地址
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;

  std::strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  // 连接 Server
  if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    perror("connect");
    return 1;
  }

  // ===== RPC 调用 =====

  std::string request = "add 10 20";

  write(fd, request.data(), request.size());

  // 接收 RPC 返回
  char buffer[1024] = {0};

  int n = read(fd, buffer, sizeof(buffer) - 1);

  if (n > 0) {
    std::cout << "RPC Result: " << std::string(buffer, n) << std::endl;
  }

  close(fd);

  return 0;
}