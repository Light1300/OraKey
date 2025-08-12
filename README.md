# OraKey – Day 1: Project Skeleton + Networking + RESP Parser

## 🎯 Day 1 Goal
Build a **working TCP server in C++** that:
1. Accepts multiple client connections (non-blocking, epoll-based).
2. Reads and parses **RESP** (Redis Serialization Protocol) requests.
3. Responds correctly to at least:
   - `PING` → `+PONG\r\n`
   - `ECHO <message>` → `<message>` in RESP format.

---


## 🗂️ Today's Tasks


### 2. Configuration
- [ ] Add basic config constants:
- Default port: `6379`
- (Optional) AOF file path (not used yet)
- Log level (INFO/WARN/ERROR)

---

### 3. Networking – TCP Server
- [ ] Create server socket:
- `socket()`, `bind()`, `listen()`.
- Set to **non-blocking** mode.
- [ ] Use **epoll** for event-driven IO:
- `epoll_create1()`
- Register server socket with `EPOLLIN`.
- [ ] Accept new clients:
- On accept, set client socket to **non-blocking**.
- Maintain `Connection` struct:
  ```cpp
  struct Connection {
      int fd;
      std::string readBuffer;
      std::string writeBuffer;
  };
  ```
- Store connections in a `std::unordered_map<int, Connection>`.

---

### 4. Reading Data
- [ ] Implement robust read loop:
- Read available bytes into `readBuffer`.
- Try parsing RESP messages from buffer.
- If incomplete, wait for more data.
- If complete, dispatch to command handler.

---

### 5. RESP Parser
- [ ] Parse **RESP types**:
- Simple String (`+OK\r\n`)
- Error (`-ERR msg\r\n`)
- Integer (`:123\r\n`)
- Bulk String (`$<len>\r\n<data>\r\n`)
- Array (`*<n>\r\n...`)
- [ ] Implement **incremental parsing**:
- Function returns:
  - Parsed command as `std::vector<std::string>`, OR
  - Signal "incomplete" if more bytes needed.
- [ ] Implement reply serializer:
- Simple String: `+OK\r\n`
- Error: `-ERR msg\r\n`
- Integer: `:<num>\r\n`
- Bulk String: `$<len>\r\n<data>\r\n`
- Array: `*<n>\r\n...`

---

### 6. Command Handlers
- [ ] Implement:
- `PING` → `+PONG\r\n`
- `ECHO <message>` → bulk string reply with `<message>`
- [ ] Command dispatch table:
```cpp
using CommandHandler = std::function<std::string(const std::vector<std::string>&)>;
std::unordered_map<std::string, CommandHandler> commandTable;

---
``` 

### ✅ End of Day 1 Deliverables
### By the end of today, you should have:

- C++ TCP server running on port 6379 using epoll.

- RESP parser fully functional (arrays + bulk strings at minimum).

- PING/ECHO commands working end-to-end.

- Unit tests for RESP parser passing.

- Verified client interaction using redis-cli or Python test.





### RUN IT WITH the following COMMAND
``` BASH
mkdir -p build
g++ -std=c++17 -pthread -Iinclude $(find src -name "*.cpp") -o build/my_redis_server
./build/my_redis_server 6380
```