#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

namespace rpc {

// ========================================
// 普通函数
// ========================================

template <typename T> struct function_traits;

// R(Args...)
template <typename R, typename... Args> struct function_traits<R(Args...)> {
  using return_type = R;

  using args_tuple = std::tuple<Args...>;

  static constexpr size_t arity = sizeof...(Args);
};

// 函数指针
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> : function_traits<R(Args...)> {};

// std::function
template <typename R, typename... Args>
struct function_traits<std::function<R(Args...)>>
    : function_traits<R(Args...)> {};

// Lambda / Functor
template <typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

// Lambda operator()
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {
};

// 非 const operator()
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {};

} // namespace rpc