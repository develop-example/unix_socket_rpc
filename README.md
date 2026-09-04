## Unix socket RPC
minimal example for explain unix socket rpc 

update:
- 升级消息协议层, 改成Length-Prefixed Protocol（长度前缀协议）
- 增加 RpcClient / RpcServer 封装

### 1. build
```
mkdir build
cd build
cmake ..
make
```

### 2. test
```
# one terminal
./server

# the other terminal
./client
```

### 3. 原理
```
RpcRequest
    │
    ├── id
    ├── method
    └── params


RpcResponse
    │
    ├── id
    ├── result
    └── error


┌──────────────────────────────┐
│        Application           │
│                              │
│ client.call("add", 10, 20)   │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│          RPC Layer           │
│                              │
│ Request / Response           │
│ Method / Params / ID         │
└──────────────┬───────────────┘
               │ JSON
               ▼
┌──────────────────────────────┐
│       Message Framing        │
│                              │
│ [Length][Payload]            │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│      Unix Domain Socket      │
└──────────────────────────────┘
```