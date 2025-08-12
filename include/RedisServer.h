#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <string>
#include <atomic>

class RedisServer {
public:
    explicit RedisServer(int port);
    void run();
    void shutdown();

private:
    int port;
    int server_socket;
    std::atomic<bool> running;

    // setup singla handeling for smooth shutdown 
    void setupSignalHandler();
};

#endif
