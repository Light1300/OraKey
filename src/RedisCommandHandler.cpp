#include "RedisCommandHandler.h"
#include "Database.h"

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <cctype>

// Parse RESP array or plain text to vector<string>
// RESP handled: *N\r\n$len\r\n<data>\r\n...
std::vector<std::string> RedisCommandHandler::parseRespCommand(const std::string &input) {
    std::vector<std::string> tokens;
    if (input.empty()) return tokens;

    // Plain-text fallback (useful for telnet during early testing)
    if (input[0] != '*') {
        std::istringstream iss(input);
        std::string token;
        while (iss >> token) tokens.push_back(token);
        return tokens;
    }

    size_t pos = 1; // skip '*'
    try {
        size_t crlf = input.find("\r\n", pos);
        if (crlf == std::string::npos) return tokens;

        int numElements = std::stoi(input.substr(pos, crlf - pos));
        pos = crlf + 2;

        for (int i = 0; i < numElements; ++i) {
            if (pos >= input.size() || input[pos] != '$') break;
            pos++; // skip '$'

            crlf = input.find("\r\n", pos);
            if (crlf == std::string::npos) break;

            int len = std::stoi(input.substr(pos, crlf - pos));
            pos = crlf + 2;

            if (pos + len > input.size()) break;

            tokens.push_back(input.substr(pos, len));
            pos += len + 2; // skip data and CRLF
        }
    } catch (const std::exception &e) {
        std::cerr << "RESP parse error: " << e.what() << std::endl;
    }
    return tokens;
}

// Constructor - store reference to Database instance (usually Database::getInstance())
RedisCommandHandler::RedisCommandHandler(Database &db) : db_(db) {}

// Helpers to format RESP replies
static std::string bulkString(const std::string &s) {
    return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
}
static std::string intReply(long v) {
    return ":" + std::to_string(v) + "\r\n";
}
static std::string okReply() {
    return "+OK\r\n";
}
static std::string nilBulk() {
    return "$-1\r\n";
}

// Process commands using the database reference
std::string RedisCommandHandler::processCommand(const std::string &commandLine) {
    auto tokens = parseRespCommand(commandLine);
    if (tokens.empty()) return "-ERR empty command\r\n";

    std::string cmd = tokens[0];
    for (auto &c : cmd) c = static_cast<char>(std::toupper((unsigned char)c));

    // PING
    if (cmd == "PING") {
        if (tokens.size() >= 2) return bulkString(tokens[1]);
        return "+PONG\r\n";
    }

    // ECHO
    if (cmd == "ECHO") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'echo'\r\n";
        return bulkString(tokens[1]);
    }

    // SET key value
    if (cmd == "SET") {
        if (tokens.size() < 3) return "-ERR wrong number of arguments for 'set'\r\n";
        db_.set(tokens[1], tokens[2]);
        return okReply();
    }

    // GET key
    if (cmd == "GET") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'get'\r\n";
        auto v = db_.get(tokens[1]);
        if (v.has_value()) return bulkString(*v);
        return nilBulk();
    }

    // DEL key
    if (cmd == "DEL") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'del'\r\n";
        bool removed = db_.del(tokens[1]);
        return intReply(removed ? 1 : 0);
    }

    // INCR key
    if (cmd == "INCR") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'incr'\r\n";
        try {
            long val = db_.incr(tokens[1]);
            return intReply(val);
        } catch (const std::exception &e) {
            return std::string("-ERR ") + e.what() + "\r\n";
        }
    }

    // LPUSH key v1 v2 ...
    if (cmd == "LPUSH") {
        if (tokens.size() < 3) return "-ERR wrong number of arguments for 'lpush'\r\n";
        std::vector<std::string> values(tokens.begin() + 2, tokens.end());
        size_t newLen = db_.lpush(tokens[1], values);
        return intReply(static_cast<long>(newLen));
    }

    // LPOP key
    if (cmd == "LPOP") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'lpop'\r\n";
        auto v = db_.lpop(tokens[1]);
        if (v.has_value()) return bulkString(*v);
        return nilBulk();
    }

    // QUIT (client side disconnect)
    if (cmd == "QUIT") {
        return okReply();
    }

    return "-ERR unknown command\r\n";
}
