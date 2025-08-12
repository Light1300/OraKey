#ifndef DATABASE_H
#define DATABASE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <mutex>

class Database {
public:
    static Database& getInstance();

    // String commands
    bool set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    long incr(const std::string& key);

    // List commands
    size_t lpush(const std::string& key, const std::vector<std::string>& values);
    std::optional<std::string> lpop(const std::string& key);

    // Persistence
    bool dump(const std::string& filename);
    bool load(const std::string& filename);

private:
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    std::mutex db_mutex;

    // Data stores
    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;
};

#endif // DATABASE_H
