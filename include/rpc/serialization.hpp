#pragma once

#include "rpc/rpc_message.hpp"

#include <stdexcept>
#include <tuple>
#include <utility>

namespace rpc {

// ========================================
// Args... -> JSON
// ========================================

template <typename... Args> json make_params(Args &&...args) {
  json params = json::array();

  (params.push_back(std::forward<Args>(args)), ...);

  return params;
}

// ========================================
// JSON -> Tuple
// ========================================

template <typename Tuple, size_t... I>
Tuple json_to_tuple_impl(const json &params, std::index_sequence<I...>) {
  return Tuple{params.at(I).get<std::tuple_element_t<I, Tuple>>()...};
}

template <typename Tuple> Tuple json_to_tuple(const json &params) {
  constexpr size_t N = std::tuple_size_v<Tuple>;

  if (!params.is_array()) {
    throw std::runtime_error("RPC params must be JSON array");
  }

  if (params.size() != N) {
    throw std::runtime_error("RPC argument count mismatch");
  }

  return json_to_tuple_impl<Tuple>(params, std::make_index_sequence<N>{});
}

} // namespace rpc