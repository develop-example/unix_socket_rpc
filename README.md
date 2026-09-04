## Unix socket RPC
minimal example for explain unix socket rpc

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
Client API
    │
    │ add(10, 20)
    ▼
RPC Serialization
    │
    │ "add 10 20"
    ▼
Unix Socket
    │
    ▼
Server
    │
    │ method == "add"
    ▼
真实函数
    │
    │ return a + b
    ▼
Response
    │
    │ "30"
    ▼
Unix Socket
```