#include "rpc/rpc_client.hpp"

#include "rpc/protocol.hpp"
#include "rpc/rpc_message.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

RpcClient::RpcClient(std::string socket_path)
    : socket_path_(std::move(socket_path)) {}

int RpcClient::call(const std::string &method, int a, int b) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (fd < 0) {
    throw std::runtime_error("Failed to create socket");
  }

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;

  std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {

    close(fd);

    throw std::runtime_error("Failed to connect RPC server");
  }

  // ============================
  // 创建 RPC Request
  // ============================

  rpc::RpcRequest request{
      .id = next_request_id_++, .method = method, .params = {a, b}};

  // Request -> JSON
  rpc::json request_json = request;

  std::string request_payload = request_json.dump();

  // ============================
  // Send
  // ============================

  if (!rpc::send_message(fd, request_payload)) {

    close(fd);

    throw std::runtime_error("Failed to send RPC request");
  }

  // ============================
  // Receive
  // ============================

  std::string response_payload;

  if (!rpc::recv_message(fd, response_payload)) {

    close(fd);

    throw std::runtime_error("Failed to receive RPC response");
  }

  close(fd);

  // ============================
  // JSON -> Response
  // ============================

  rpc::json response_json = rpc::json::parse(response_payload);

  rpc::RpcResponse response = response_json.get<rpc::RpcResponse>();

  // 校验 Response ID
  if (response.id != request.id) {
    throw std::runtime_error("RPC response ID mismatch");
  }

  // RPC Error
  if (response.error) {
    throw std::runtime_error(response.error->message);
  }

  // 返回结果
  return response.result.get<int>();
}