## Unix socket RPC
minimal example for explain unix socket rpc 

update:
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
- client
```
call()
 │
 ├── socket()
 ├── connect()
 ├── send()
 ├── recv()
 └── close()

->

RpcClient()
 │
 ├── socket()
 └── connect()

call()
 │
 ├── send()
 └── recv()

call()
 │
 ├── send()
 └── recv()

~RpcClient()
 │
 └── close()

```
- server
```
accept()
  │
  ├── recv
  ├── process
  ├── send
  └── close

->

run()
 │
 │ accept()
 │
 ├──────── Client A
 │            │
 │            ▼
 │       handle_client()
 │            │
 │            ├── Request 1
 │            ├── Response 1
 │            │
 │            ├── Request 2
 │            ├── Response 2
 │            │
 │            └── Request 3
 │
 ├──────── Client B
 │
 └──────── Client C

```