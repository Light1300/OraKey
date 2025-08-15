#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>
#include <vector>

class Database; // forward declaration

class RedisCommandHandler {
public:
    explicit RedisCommandHandler(Database &db);

    // Parse RESP or plain text commands into vector<string>
    static std::vector<std::string> parseRespCommand(const std::string &input);

    // Process one command (RESP or plain text) -> RESP reply
    std::string processCommand(const std::string &commandLine);

private:
    Database &db_;
};

#endif // REDIS_COMMAND_HANDLER_H
