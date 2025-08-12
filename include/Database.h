#ifndef DATABASE_H
#define DATABASE_H

#include <unordered_map>
#include <string>
#include <list>
#include <variant>
#include <optional>
#include <mutex>
#include <unordered_map>

using Value = std::variant<std::string, std::list<std::string>, int64_t>;

class Database {
public:
    // getting singleton  instance
    static Database& getInstance();

    // common command
    bool flushALL();
    bool set(const std::string& key, const std::string& value);
    bool get(const std::string& key, const std::string& value);
    std::vector<std::string> keys();
    std::string type(const std::string&key);
    bool del(const std::string&key);

    //expire 
    //rename
    
    // bool del(const std::string& key);
    // long incr(const std::string& key);
    // size_t lpush(const std::string& key, const std::vector<std::string>& values);
    // std::optional<std::string> lpop(const std::string& key);

    


    // persistance : Dump / load the database from a file.
    bool dump(const std::string& filename);
    bool load(const std::string& filename);


private:
    std::unordered_map<std::string, Value> store_;
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    std::mutex db_mutex;
    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> ls_store;
    stdL::unordered_map<std::string, std::unordered_map<std::string,std::string>> hash_store;
};

#endif
