## Unix socket RPC
minimal example for explain unix socket rpc 

update:
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
┌──────────────────────────────┐
│        Application           │
│                              │
│ client.call("add", 10, 20)   │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│         RpcClient            │
│                              │
│ Serialize Request            │
│ Connect Socket               │
│ Send / Receive               │
└──────────────┬───────────────┘
               │
               ▼
         Unix Socket
               │
               ▼
┌──────────────────────────────┐
│         RpcServer            │
│                              │
│ Parse Request                │
│ Find Handler                 │
│ Execute Function             │
└──────────────┬───────────────┘
               │
               ▼
        handlers_
        ┌───────────────┐
        │ add      ───► │ lambda
        │ multiply ───► │ lambda
        └───────────────┘
```