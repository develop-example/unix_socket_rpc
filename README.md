## Unix socket RPC
minimal example for explain unix socket rpc 

update:
- 线程池
- 异步RPC
- 多线程并发
- 模板化rpc client/server
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
- RPC server架构
```
                         RpcServer
                             │
                         accept()
                             │
                 ┌───────────┴───────────┐
                 │                       │
           Connection A             Connection B
                 │                       │
                 ▼                       ▼
           Receive Thread          Receive Thread
                 │                       │
                 └───────────┬───────────┘
                             │
                             ▼
                      ┌────────────┐
                      │ Task Queue │
                      └──────┬─────┘
                             │
             ┌───────────────┼───────────────┐
             ▼               ▼               ▼
         Worker 1        Worker 2        Worker N
             │               │               │
             ▼               ▼               ▼
          Handler         Handler         Handler
             │               │               │
             └───────────────┼───────────────┘
                             │
                             ▼
                    ConnectionContext
                             │
                       send_mutex
                             │
                             ▼
                          Socket
```

