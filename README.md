## Unix socket RPC
minimal example for explain unix socket rpc 

update:
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
- 异步数据流
```
┌─────────────────────────────────────────────────────┐
│                     RpcClient                       │
│                                                     │
│ call_async<int>()                                  │
│        │                                            │
│        ▼                                            │
│ Request ID                                          │
│        │                                            │
│        ▼                                            │
│ Pending Map                                         │
│        │                                            │
│        ▼                                            │
│ Promise / Future                                    │
│        │                                            │
│        ▼                                            │
│ send_mutex                                          │
└────────┼────────────────────────────────────────────┘
         │
         ▼
════════════════ Unix Socket ════════════════════════
         │
         ▼
┌─────────────────────────────────────────────────────┐
│                     RpcServer                       │
│                                                     │
│              Connection Receive Thread              │
│                         │                           │
│                         ▼                           │
│                   recv Request                      │
│                         │                           │
│              ┌──────────┼──────────┐                │
│              │          │          │                │
│              ▼          ▼          ▼                │
│           Worker 1   Worker 2   Worker 3            │
│              │          │          │                │
│              ▼          ▼          ▼                │
│            Handler    Handler    Handler            │
│              │          │          │                │
│              └──────────┼──────────┘                │
│                         │                           │
│                         ▼                           │
│                   ConnectionContext                 │
│                         │                           │
│                    send_mutex                       │
└─────────────────────────┼───────────────────────────┘
                          │
                          ▼
════════════════ Unix Socket ════════════════════════
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│                 Client Receive Thread               │
│                                                     │
│ Response ID                                         │
│     │                                               │
│     ▼                                               │
│ Pending Map                                         │
│     │                                               │
│     ▼                                               │
│ Promise.set_value()                                 │
└─────────────────────────────────────────────────────┘
```

