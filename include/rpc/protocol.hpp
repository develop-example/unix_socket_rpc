#pragma once

#include <string>

namespace rpc {

// 发送完整消息
bool send_message(int fd, const std::string &message);

// 接收完整消息
bool recv_message(int fd, std::string &message);

} // namespace rpc