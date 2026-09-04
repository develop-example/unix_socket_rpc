#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

constexpr const char *SOCKET_PATH = "/tmp/demo_rpc.sock";

int main() {
  // 删除旧 socket 文件
  unlink(SOCKET_PATH);

  // 创建 Unix Socket
  int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    return 1;
  }

  // 配置地址
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  // bind
  if (bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }

  // listen
  if (listen(server_fd, 10) < 0) {
    perror("listen");
    return 1;
  }

  std::cout << "RPC Server listening on " << SOCKET_PATH << std::endl;

  while (true) {
    // 等待 Client
    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    char buffer[1024] = {0};

    int n = read(client_fd, buffer, sizeof(buffer) - 1);

    if (n > 0) {
      std::string request(buffer, n);

      std::cout << "Request: " << request << std::endl;

      // ===== 简单 RPC 协议 =====
      //
      // 请求:
      // add 10 20
      //
      // 响应:
      // 30
      //
      std::istringstream iss(request);

      std::string method;
      int a, b;

      iss >> method >> a >> b;

      std::string response;

      if (method == "add") {
        int result = a + b;
        response = std::to_string(result);
      } else {
        response = "ERROR: unknown method";
      }

      write(client_fd, response.data(), response.size());
    }

    close(client_fd);
  }

  close(server_fd);
  unlink(SOCKET_PATH);

  return 0;
}