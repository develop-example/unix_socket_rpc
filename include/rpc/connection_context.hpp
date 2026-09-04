#pragma once

#include <atomic>
#include <mutex>

namespace rpc {

struct ConnectionContext {

  explicit ConnectionContext(int socket_fd) : fd(socket_fd) {}

  // Client Socket
  int fd = -1;

  // Serialize response writes
  std::mutex send_mutex;

  // Connection state
  std::atomic<bool> alive{true};
};

} // namespace rpc