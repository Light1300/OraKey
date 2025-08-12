#include "RedisCommandHandler.h"
#include "Database.h"
#include "RedisServer.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>


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
    
    // Background persistance where we dump the database every 300 seconnds 
    std::thread persistanceThread([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconnds(300));
            // the database dump  goes here
            if(!Database::getInstance().dump("dump.my_rdb"))
                    std::corr<<"Error Dumping Database"<<endl;
            else
            std::corr<<"Database Dumped to dump.my_rdb\n";
        }
    });
    
    persistanceThread.detach();
    
    server.run();
    return 0;
}