#pragma once

#include "rpc/function_traits.hpp"
#include "rpc/rpc_message.hpp"
#include "rpc/serialization.hpp"

#include <atomic>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

class RpcServer {
public:
  explicit RpcServer(std::string socket_path);

  ~RpcServer();

  RpcServer(const RpcServer &) = delete;
  RpcServer &operator=(const RpcServer &) = delete;

  template <typename Func>
  void register_method(const std::string &name, Func &&func) {
    using FuncType = std::decay_t<Func>;

    using Traits = rpc::function_traits<FuncType>;

    using ReturnType = typename Traits::return_type;

    using ArgsTuple = typename Traits::args_tuple;

    handlers_[name] = [func = std::forward<Func>(func)](
                          const rpc::json &params) mutable -> rpc::json {
      auto args = rpc::json_to_tuple<ArgsTuple>(params);

      if constexpr (std::is_void_v<ReturnType>) {

        std::apply(func, args);

        return nullptr;

      } else {

        return std::apply(func, args);
      }
    };
  }

  void run();

  void stop();

private:
  using Handler = std::function<rpc::json(const rpc::json &)>;

  // 一个 Client Connection
  // 对应一个处理循环
  void handle_client(int client_fd);

private:
  std::string socket_path_;

  int server_fd_ = -1;

  std::atomic<bool> running_{false};

  std::unordered_map<std::string, Handler> handlers_;
};