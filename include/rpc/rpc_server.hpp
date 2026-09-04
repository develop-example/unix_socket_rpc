#pragma once

#include "rpc/connection_context.hpp"
#include "rpc/function_traits.hpp"
#include "rpc/rpc_message.hpp"
#include "rpc/serialization.hpp"
#include "rpc/thread_pool.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

class RpcServer {
public:
  explicit RpcServer(std::string socket_path, std::size_t worker_threads);

  ~RpcServer();

  RpcServer(const RpcServer &) = delete;

  RpcServer &operator=(const RpcServer &) = delete;

  // ========================================
  // Typed RPC Registration
  // ========================================

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

  // 一个 Connection 一个 Receive Loop
  void handle_client(std::shared_ptr<rpc::ConnectionContext> connection);

  // 一个 Request 一个 Worker Task
  void process_request(std::shared_ptr<rpc::ConnectionContext> connection,

                       rpc::RpcRequest request);

private:
  std::string socket_path_;

  int server_fd_ = -1;

  std::atomic<bool> running_{false};

  std::unordered_map<std::string, Handler> handlers_;

  std::unique_ptr<rpc::ThreadPool> thread_pool_;
};