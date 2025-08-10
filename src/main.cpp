#include "../include/RedisServer.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    int port = 6380; // default port
    try {
        if (argc >= 2) {
            port = std::stoi(argv[1]);
        }
    } catch (...) {
        std::cerr << "Invalid port number. Using default 6379.\n";
    }

    RedisServer server(port);
    server.run();

    return 0;
}
