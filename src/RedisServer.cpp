#include "RedisServer.h"
#include "RedisCommandHandler.h"
#include "Database.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>
#include <csignal>

static RedisServer* g_server_ptr = nullptr;

static void signal_handler(int signum) {
    if (g_server_ptr) {
        std::cout << "\nSignal " << signum << " received. Shutting down server...\n";
        g_server_ptr->shutdown();
    }
}

RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true) {
    g_server_ptr = this;
    setupSignalHandler();
}

void RedisServer::setupSignalHandler() {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);
}

void RedisServer::shutdown() {
    running = false;
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
}

void RedisServer::run() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << "\n";
        return;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setsockopt: " << strerror(errno) << "\n";
        close(server_socket);
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(port));
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding: " << strerror(errno) << "\n";
        close(server_socket);
        return;
    }

    if (listen(server_socket, 10) < 0) {
        std::cerr << "Error listening: " << strerror(errno) << "\n";
        close(server_socket);
        return;
    }

    std::cout << "Server listening on port " << port << "\n";

    Database &db = Database::getInstance();
    RedisCommandHandler handler(db);

    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientlen = sizeof(clientAddr);
        int clientSock = accept(server_socket, reinterpret_cast<sockaddr*>(&clientAddr), &clientlen);
        if (clientSock < 0) {
            if (errno == EINTR) continue;
            std::cerr << "Accept error: " << strerror(errno) << "\n";
            continue;
        }

        char ipbuf[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipbuf, sizeof(ipbuf));
        uint16_t clientPort = ntohs(clientAddr.sin_port);
        std::cout << "Client connected: " << ipbuf << ":" << clientPort << "\n";

        constexpr size_t BUF_SZ = 4096;
        std::string bufferStr;
        bufferStr.reserve(4096);
        char buf[BUF_SZ];

        bool clientAlive = true;
        while (clientAlive && running) {
            ssize_t n = recv(clientSock, buf, BUF_SZ, 0);
            if (n > 0) {
                bufferStr.append(buf, static_cast<size_t>(n));

                // For simplicity, treat buffer as one complete command
                std::string reply = handler.processCommand(bufferStr);

                // send reply
                size_t sent = 0;
                const char* out = reply.c_str();
                size_t tosend = reply.size();
                while (sent < tosend) {
                    ssize_t w = send(clientSock, out + sent, tosend - sent, 0);
                    if (w <= 0) { clientAlive = false; break; }
                    sent += static_cast<size_t>(w);
                }

                // QUIT detection
                auto toks = RedisCommandHandler::parseRespCommand(bufferStr);
                if (!toks.empty()) {
                    std::string c = toks[0];
                    for (auto &ch : c) ch = static_cast<char>(std::toupper((unsigned char)ch));
                    if (c == "QUIT" || c == "EXIT") clientAlive = false;
                }
                bufferStr.clear();
            } else if (n == 0) {
                clientAlive = false; // client closed
            } else {
                if (errno == EINTR) continue;
                std::cerr << "Recv error: " << strerror(errno) << "\n";
                clientAlive = false;
            }
        }

        close(clientSock);
        std::cout << "Client disconnected: " << ipbuf << ":" << clientPort << "\n";
    }

    if (!Database::getInstance().dump("dump.my_rdb")) {
        std::cerr << "Error dumping database\n";
    } else {
        std::cout << "Database dumped to dump.my_rdb\n";
    }
}
