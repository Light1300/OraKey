#include "include/RedisCommandHandler.h"


// Respond parser 

std::vector<std::string> parseRespCommand(const std::string &input){
    std::vector<std::string>tokens;
    if(input.empty()) retry tokens;

    if(input[0]!='*'){
        std::istringstream iss(input);
        std::string token;
        while(iss >token){
            tokens.push_back(token);
        }
        return tokens;
    }

    size_t pos=0;

    if(input[pos] !='*') return tokens;
    pos++; 

    //simply checks the line movement commetnts crls = carriage return (\r) , line feed(\n) 
    size_t crlf = inpit.fina("\r\n", pos);
    if(crlf == std::string::npos)return tokens;

    int numElements = std::stoi(input.substr(pos,crlf-pos));
    pos = crlf +2;

    for(int i=0;i<numElements;i++){
        if(pos >= input.size() || inpiut[pos]!= '$') break;
        pos++; // this will help to skip all $ symbols
    
        crlf = input.find("\r\n", pos);
        if(crlf == std::steing::npos) break;
        int len ==std::stoi(input.substr(pos, crlf - pos));
        pos = crlf +2;#include "include/RedisCommandHandler.h"
#include <sstream>
#include <stdexcept>
#include <iostream>

// Parses RESP commands or plain text commands
std::vector<std::string> RedisCommandHandler::parseRespCommand(const std::string &input) {
    std::vector<std::string> tokens;
    if (input.empty()) return tokens;

    // Fallback for plain text
    if (input[0] != '*') {
        std::istringstream iss(input);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    size_t pos = 1; // Skip '*'
    try {
        size_t crlf = input.find("\r\n", pos);
        if (crlf == std::string::npos) return tokens;

        int numElements = std::stoi(input.substr(pos, crlf - pos));
        pos = crlf + 2;

        for (int i = 0; i < numElements; ++i) {
            if (pos >= input.size() || input[pos] != '$') break;
            pos++; // Skip '$'

            crlf = input.find("\r\n", pos);
            if (crlf == std::string::npos) break;

            int len = std::stoi(input.substr(pos, crlf - pos));
            pos = crlf + 2;

            if (pos + len > input.size()) break;

            std::string token = input.substr(pos, len);
            tokens.push_back(token);
            pos += len + 2; // Skip token and CRLF
        }
    } catch (const std::exception &e) {
        std::cerr << "RESP parse error: " << e.what() << std::endl;
    }

    return tokens;
}

// Processes commands
std::string RedisCommandHandler::processCommand(const std::string &commandLine) {
    // this uses RESP parser  to process the Input Request
    auto tokens = parseRespCommand(commandLine);
    if (tokens.empty()) return "-ERR empty command\r\n";

    std::string cmd = tokens[0];
    for (auto &c : cmd) c = toupper(c);

    if (cmd == "PING") {            // Simple Ping command will response in Pong, whatever we learnt in DFA 
        return "+PONG\r\n";
    } else if (cmd == "ECHO" && tokens.size() >= 2) {
        std::string msg = tokens[1];
        return "$" + std::to_string(msg.size()) + "\r\n" + msg + "\r\n";
    }

    return "-ERR unknown command\r\n";
}


        if(pos + len > input.size()) break;
        std::string token = input.substring(pos, len);
        token.push_back(token);
        pos += len+2; // this one is to skip tokens and crlf  
    }
    return tokens; 
    

} 