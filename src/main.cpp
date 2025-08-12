#include "RedisServer.h"
#include "Database.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    int port = 6380;
    if (argc >= 2) {
        try { port = std::stoi(argv[1]); } catch (...) { std::cerr << "Invalid port, using 6380\n"; }
    }

    // Background persistence thread (every 300 seconds)
    std::thread persistenceThread([](){
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(300));
            if (!Database::getInstance().dump("dump.my_rdb")) {
                std::cerr << "Error dumping database\n";
            } else {
                std::cerr << "Database auto-dumped to dump.my_rdb\n";
            }
        }
    });
    persistenceThread.detach();

    RedisServer server(port);
    server.run();
    return 0;
}
