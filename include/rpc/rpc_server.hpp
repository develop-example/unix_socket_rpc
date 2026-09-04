#pragma once

#include "rpc/function_traits.hpp"
#include "rpc/rpc_message.hpp"
#include "rpc/serialization.hpp"

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

  // ========================================
  // 注册 Typed RPC
  // ========================================

  template <typename Func>
  void register_method(const std::string &name, Func &&func) {
    using FuncType = std::decay_t<Func>;

    using Traits = rpc::function_traits<FuncType>;

    using ReturnType = typename Traits::return_type;

    using ArgsTuple = typename Traits::args_tuple;

    // 将 Typed Function
    //
    // R(Args...)
    //
    // 包装成：
    //
    // json(json)
    //
    handlers_[name] = [func = std::forward<Func>(func)](
                          const rpc::json &params) mutable -> rpc::json {
      // JSON -> Tuple<Args...>
      auto args = rpc::json_to_tuple<ArgsTuple>(params);

      if constexpr (std::is_void_v<ReturnType>) {

        // 调用 void 函数
        std::apply(func, args);

        return nullptr;

      } else {

        // 调用有返回值函数
        ReturnType result = std::apply(func, args);

        // C++ Type -> JSON
        return result;
      }
    };
  }

  void run();

private:
  using Handler = std::function<rpc::json(const rpc::json &)>;

  std::string socket_path_;

  int server_fd_ = -1;

  std::unordered_map<std::string, Handler> handlers_;
};