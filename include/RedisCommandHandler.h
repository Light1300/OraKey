#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>
#include <vector>
#include <optional>

class Database; // forward declaration

class RedisCommandHandler {
public:
    // Construct with a reference to the in-memory database
    explicit RedisCommandHandler(Database &db);

    // Parse RESP or plain text commands into vector<string>
    static std::vector<std::string> parseRespCommand(const std::string &input);

    // Process a single command line (RESP or plain text) and return a RESP-formatted reply
    std::string processCommand(const std::string &commandLine);

private:
    Database &db_;
};

#endif // REDIS_COMMAND_HANDLER_H
