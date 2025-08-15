#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <atomic>

class RedisServer {
public:
    explicit RedisServer(int port);
    ~RedisServer() = default;

    void run();
    void shutdown();
    void setupSignalHandler();

private:
    int port;
    int server_socket = -1;
    std::atomic<bool> running{false};
};

#endif // REDIS_SERVER_H
