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