// #include "RedisServer.h"
#include "include/RedisServer.h"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <cstring> // for strerror
#include <cerrno>  // for errno

static RedisServer* globalServer = nullptr;

RedisServer::RedisServer(int port)
    : port(port), server_socket(-1), running(true) {
    globalServer = this;
}

void RedisServer::shutdown() {
    running = false;
    if (server_socket != -1) {
        close(server_socket);
    }
    std::cout << "Server has been shutdown.\n";
}

void RedisServer::run() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating socket\n";
        return;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options\n";
        close(server_socket);
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding server socket: " << strerror(errno) << "\n";
        close(server_socket);
        return;
    }

    if (listen(server_socket, 10) < 0) {
        std::cerr << "Error: Server socket is not listening: " << strerror(errno) << "\n";
        close(server_socket);
        return;
    }

    std::cout << "Server is listening on port " << port << std::endl;

    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientlen = sizeof(clientAddr);

        int clientSocket = accept(server_socket, (struct sockaddr*)&clientAddr, &clientlen);
        if (clientSocket < 0) {
            if (errno == EINTR) continue; // retry if interrupted
            std::cerr << "Accept failed: " << strerror(errno) << "\n";
            continue;
        }

        std::cout << "Connected to Client: " << inet_ntoa(clientAddr.sin_addr)
                  << ":" << ntohs(clientAddr.sin_port) << "\n";

        // For now, just close connection immediately
        close(clientSocket);
    }
}
