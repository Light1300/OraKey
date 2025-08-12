// src/RedisCommandHandler.cpp
#include "RedisCommandHandler.h"
#include "Database.h"

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <cctype>   // toupper

// Parse RESP or plain-text into vector<string>
// RESP (simple subset): Array of Bulk Strings: *N\r\n$len\r\n<data>\r\n...
std::vector<std::string> RedisCommandHandler::parseRespCommand(const std::string &input) {
    std::vector<std::string> tokens;
    if (input.empty()) return tokens;

    // Plain-text fallback: split by whitespace (useful for telnet during development)
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

            std::string token = input.substr(pos, len);
            tokens.push_back(token);
            pos += len + 2; // skip token and CRLF
        }
    } catch (const std::exception &e) {
        std::cerr << "RESP parse error: " << e.what() << std::endl;
    }
    return tokens;
}

// Constructor that takes a reference to the in-memory database.
RedisCommandHandler::RedisCommandHandler(Database &db) : db_(db) {}

// Helper: format bulk string response
static std::string bulkString(const std::string &s) {
    return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
}

// Helper: format integer response
static std::string intReply(long v) {
    return ":" + std::to_string(v) + "\r\n";
}

// Process high-level command (uses Database)
std::string RedisCommandHandler::processCommand(const std::string &commandLine) {
    auto tokens = parseRespCommand(commandLine);
    if (tokens.empty()) return "-ERR empty command\r\n";
    
    // Normalize command to uppercase
    std::string cmd = tokens[0];
    Database& db = Database::getInstance();
    for (auto &c : cmd) c = static_cast<char>(std::toupper((unsigned char)c));

    if (cmd == "PING") {
        if (tokens.size() >= 2) return bulkString(tokens[1]); // PING msg -> echo msg
        return "+PONG\r\n";
    }

    if (cmd == "ECHO") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'echo'\r\n";
        return bulkString(tokens[1]);
    }

    // Key-value commands that use Database
    if (cmd == "SET") {
        if (tokens.size() < 3) return "-ERR wrong number of arguments for 'set'\r\n";
        bool ok = db_.set(tokens[1], tokens[2]);
        return ok ? "+OK\r\n" : "-ERR set failed\r\n";
    }

    if (cmd == "GET") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'get'\r\n";
        auto v = db_.get(tokens[1]);
        if (v.has_value()) return bulkString(*v);
        return "$-1\r\n"; // nil bulk string
    }

    if (cmd == "DEL") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'del'\r\n";
        bool removed = db_.del(tokens[1]);
        return intReply(removed ? 1 : 0);
    }

    if (cmd == "INCR") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'incr'\r\n";
        try {
            long val = db_.incr(tokens[1]);
            return intReply(val);
        } catch (const std::exception &e) {
            return std::string("-ERR ") + e.what() + "\r\n";
        }
    }

    if (cmd == "LPUSH") {
        if (tokens.size() < 3) return "-ERR wrong number of arguments for 'lpush'\r\n";
        // tokens[2..] are values
        std::vector<std::string> values(tokens.begin() + 2, tokens.end());
        size_t newLen = db_.lpush(tokens[1], values);
        return intReply(static_cast<long>(newLen));
    }

    if (cmd == "LPOP") {
        if (tokens.size() < 2) return "-ERR wrong number of arguments for 'lpop'\r\n";
        auto v = db_.lpop(tokens[1]);
        if (v.has_value()) return bulkString(*v);
        return "$-1\r\n";
    }

    if (cmd == "QUIT") {
        return "+OK\r\n"; // server will close connection after sending this
    }



    return "-ERR unknown command\r\n";
}
