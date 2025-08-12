#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <atomic>

class RedisServer {
public:
    explicit RedisServer(int port);
    ~RedisServer() = default;

    // Start accept loop (blocking)
    void run();

    // Request a graceful shutdown (can be called from signal handler)
    void shutdown();

    // Set up OS signal handler (SIGINT)
    void setupSignalHandler();

private:
    int port;
    int server_socket = -1;
    std::atomic<bool> running{false};
};

#endif // REDIS_SERVER_H
