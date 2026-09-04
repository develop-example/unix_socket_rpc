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
Application
    │
    │ call("add", 10, 20)
    ▼
RpcClient
    │
    │ Serialize
    ▼
"add 10 20"
    │
    ▼
Protocol
    │
    ├── Length = 9
    │
    └── Payload = "add 10 20"
    │
    ▼
┌─────────────┬──────────────┐
│ 00 00 00 09│ add 10 20    │
└─────────────┴──────────────┘
    │
    ▼
Unix Domain Socket
    │
    ▼
Protocol
    │
    ├── read 4 bytes
    │
    ├── length = 9
    │
    └── read 9 bytes
    ▼
"add 10 20"
    │
    ▼
RpcServer
    │
    ├── Parse Method
    │
    ├── Find Handler
    │
    └── Execute
    ▼
30
```