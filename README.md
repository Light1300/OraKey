Alright — I’ll prepare a **technical README** for your current Redis clone, strictly covering the features you’ve already implemented and detailing compilation, usage, and test cases.

---

## **README.md**

```markdown
# Minimal Redis Clone (C++17)

A lightweight Redis-like key-value store implemented in C++17 with a custom RESP protocol parser, supporting a subset of Redis commands.  
This project demonstrates TCP networking, concurrent client handling, and basic in-memory database management.

---

## Features Implemented

- **RESP Protocol Parsing** – Supports standard Redis serialization protocol for client communication.
- **Supported Commands**:
  - `PING` → Health check (`+PONG`)
  - `ECHO <message>` → Returns message
  - `SET <key> <value>` → Store string value
  - `GET <key>` → Retrieve string value
  - `DEL <key>` → Delete key
  - `INCR <key>` → Increment integer value (creates if absent)
  - `LPUSH <key> <value>` → Push value to start of list
  - `LPOP <key>` → Pop value from start of list
- **Multi-client Support** – Uses `std::thread` for concurrent client connections.
- **Graceful Error Handling** – RESP-compliant error messages for unknown commands.

---

## Directory Structure

```

.
├── include
│   ├── Database.h
│   ├── RedisCommandHandler.h
│   └── RedisServer.h
├── src
│   ├── Database.cpp
│   ├── RedisCommandHandler.cpp
│   ├── RedisServer.cpp
│   └── main.cpp
└── README.md

````

---

## Compilation

Ensure you have **g++ (C++17)** installed.  
Run:

```bash
g++ -std=c++17 -pthread -Iinclude src/*.cpp -o redis_server
````

---

## Running the Server

```bash
./redis_server
```

The server starts on the configured port (default: **6379**).
It listens for TCP client connections using the Redis protocol.

---

## Connecting to the Server

You can use the official `redis-cli` or `netcat` (`nc`) for testing.

### Using `redis-cli`:

```bash
redis-cli -p 6379
```

### Using `nc` (Netcat):

```bash
nc 127.0.0.1 6379
```

Commands must be sent in RESP format or as plain text for testing.

---

## Example Test Cases

### **PING**

```
PING
```

Response:

```
+PONG
```

### **ECHO**

```
ECHO HelloWorld
```

Response:

```
$10
HelloWorld
```

### **SET / GET**

```
SET name Sarvesh
GET name
```

Response:

```
+OK
$7
Sarvesh
```

### **DEL**

```
DEL name
```

Response:

```
:1
```

### **INCR**

```
SET counter 10
INCR counter
```

Response:

```
:11
```

### **LPUSH / LPOP**

```
LPUSH mylist first
LPUSH mylist second
LPOP mylist
```

Response:

```
:1
:2
$6
second
```

---

## Limitations

* No key expiry (`EXPIRE`) yet.
* No persistence to disk.
* No advanced data structures beyond strings and lists.
* No authentication (`AUTH`) implemented.

---

## Notes

* This server accepts **both RESP and plain text commands** for convenience.
* Multi-client support is available but not yet optimized for high concurrency.
* All data is stored **in-memory only**.

---

## Author

Sarvesh Patil – Minimal Redis server implementation in C++17.

```

---

If you want, I can also add a **section in the README on RESP command format** so even raw TCP testers can interact without `redis-cli`.  
That way it’s fully self-contained for testing.  

Do you want me to add that section?
```
