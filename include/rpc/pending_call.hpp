#pragma once

#include "rpc/rpc_message.hpp"

#include <exception>
#include <future>
#include <stdexcept>
#include <type_traits>

namespace rpc {

// ========================================
// Pending Call Base
// ========================================

class PendingCallBase {
public:
  virtual ~PendingCallBase() = default;

  virtual void set_response(const RpcResponse &response) = 0;

  virtual void set_exception(std::exception_ptr exception) = 0;
};

// ========================================
// Pending Call<T>
// ========================================

template <typename T> class PendingCall : public PendingCallBase {

public:
  std::future<T> get_future() { return promise_.get_future(); }

  void set_response(const RpcResponse &response) override {

    try {

      if (response.error) {

        throw std::runtime_error(response.error->message);
      }

      if constexpr (std::is_void_v<T>) {

        promise_.set_value();

      } else {

        promise_.set_value(response.result.get<T>());
      }

    } catch (...) {

      promise_.set_exception(std::current_exception());
    }
  }

  void set_exception(std::exception_ptr exception) override {

    try {

      promise_.set_exception(exception);

    } catch (const std::future_error &) {

      // Promise 已经完成
    }
  }

private:
  std::promise<T> promise_;
};

} // namespace rpc