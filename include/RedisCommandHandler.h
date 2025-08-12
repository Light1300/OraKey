#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>
#include <vector>

class RedisCommandHandler {
public:
    RedisCommandHandler() = default;

    // Parse RESP or plain text commands
    static std::vector<std::string> parseRespCommand(const std::string &input);

    // Process high-level command (PING, ECHO, etc.)
    std::string processCommand(const std::string &commandLine);
};

#endif