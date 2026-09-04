#include "rpc/protocol.hpp"

#include <arpa/inet.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <limits>

namespace rpc {

namespace {

// 最大消息大小：1 MB
constexpr uint32_t MAX_MESSAGE_SIZE = 1024 * 1024;

// 保证发送全部数据
bool write_all(int fd, const void *data, size_t size) {
  const char *ptr = static_cast<const char *>(data);

  size_t remaining = size;

  while (remaining > 0) {

    ssize_t n = write(fd, ptr, remaining);

    if (n <= 0) {
      return false;
    }

    ptr += n;
    remaining -= n;
  }

  return true;
}

// 保证读取全部数据
bool read_all(int fd, void *data, size_t size) {
  char *ptr = static_cast<char *>(data);

  size_t remaining = size;

  while (remaining > 0) {

    ssize_t n = read(fd, ptr, remaining);

    if (n <= 0) {
      return false;
    }

    ptr += n;
    remaining -= n;
  }

  return true;
}

} // namespace

bool send_message(int fd, const std::string &message) {
  if (message.size() > MAX_MESSAGE_SIZE) {
    return false;
  }

  // Payload 长度
  uint32_t length = static_cast<uint32_t>(message.size());

  // 转换为网络字节序
  uint32_t network_length = htonl(length);

  // 发送消息长度
  if (!write_all(fd, &network_length, sizeof(network_length))) {
    return false;
  }

  // 发送消息内容
  if (!message.empty()) {
    if (!write_all(fd, message.data(), message.size())) {
      return false;
    }
  }

  return true;
}

bool recv_message(int fd, std::string &message) {
  uint32_t network_length = 0;

  // 读取长度
  if (!read_all(fd, &network_length, sizeof(network_length))) {
    return false;
  }

  // 网络字节序 -> Host 字节序
  uint32_t length = ntohl(network_length);

  // 防止恶意 Client 发送超大长度
  if (length > MAX_MESSAGE_SIZE) {
    return false;
  }

  // 分配 Payload 空间
  message.resize(length);

  // 读取 Payload
  if (length > 0) {
    if (!read_all(fd, message.data(), length)) {
      return false;
    }
  }

  return true;
}

} // namespace rpc