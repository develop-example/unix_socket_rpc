#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace rpc {

using json = nlohmann::json;

// ============================
// RPC Request
// ============================

struct RpcRequest {
  uint64_t id;
  std::string method;
  json params;
};

// ============================
// RPC Error
// ============================

struct RpcError {
  int code;
  std::string message;
};

// ============================
// RPC Response
// ============================

struct RpcResponse {
  uint64_t id;
  json result;

  std::optional<RpcError> error;
};

inline void to_json(json &j, const RpcRequest &request) {
  j = json{{"id", request.id},
           {"method", request.method},
           {"params", request.params}};
}

inline void from_json(const json &j, RpcRequest &request) {
  j.at("id").get_to(request.id);
  j.at("method").get_to(request.method);
  j.at("params").get_to(request.params);
}

inline void to_json(json &j, const RpcError &error) {
  j = json{{"code", error.code}, {"message", error.message}};
}

inline void from_json(const json &j, RpcError &error) {
  j.at("code").get_to(error.code);
  j.at("message").get_to(error.message);
}

inline void to_json(json &j, const RpcResponse &response) {
  j = {{"id", response.id}};

  if (response.error) {
    j["error"] = *response.error;
  } else {
    j["result"] = response.result;
  }
}

inline void from_json(const json &j, RpcResponse &response) {
  j.at("id").get_to(response.id);

  if (j.contains("error")) {
    response.error = j.at("error").get<RpcError>();
  } else {
    response.result = j.at("result");
  }
}

} // namespace rpc