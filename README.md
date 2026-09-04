## Unix socket RPC
minimal example for explain unix socket rpc 

update:
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
- client
```
C++ Arguments

10, 20
   │
   ▼
make_params()
   │
   ▼
JSON

[10, 20]
   │
   ▼
RPC Request

{
    "id": 1,
    "method": "add",
    "params": [10, 20]
}
```
- server
```
RPC Request
      │
      ▼
JSON params

[10, 20]
      │
      ▼
json_to_tuple

tuple<int, int>
      │
      ▼
std::apply

lambda(10, 20)
      │
      ▼
30
      │
      ▼
JSON Response

```
- 核心转换
```
Client Side

Args...
   ↓
JSON Array


Server Side

JSON Array
   ↓
Tuple<Args...>
   ↓
std::apply()
   ↓
Function Call
```
