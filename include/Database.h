#ifndef DATABASE_H
#define DATABASE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <chrono>

class Database {
public:
    static Database& getInstance();

    // ----- String commands -----
    bool set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    long incr(const std::string& key);
    bool exists(const std::string& key) const;

    // ----- List commands -----
    size_t lpush(const std::string& key, const std::vector<std::string>& values);
    std::optional<std::string> lpop(const std::string& key);
    // LRANGE [start, stop] inclusive (supports negatives similar to Redis)
    std::vector<std::string> lrange(const std::string& key, int start, int stop);

    // ----- Expiry & key management -----
    // returns true if expiry set; false if key doesn't exist
    bool expire(const std::string& key, int seconds);
    // TTL in seconds; -1 no expiry, -2 key doesn't exist
    long ttl(const std::string& key);
    // KEYS pattern (supports '*' and '?')
    std::vector<std::string> keys(const std::string& pattern);

    // Called by background thread and opportunistically on access
    void purgeExpired();

    // ----- Persistence -----
    bool dump(const std::string& filename);
    bool load(const std::string& filename);

private:
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Internal helpers (mutex must be held where noted)
    bool keyExistsUnlocked(const std::string& key) const;
    bool isExpiredUnlocked(const std::string& key,
                           const std::chrono::steady_clock::time_point& now) const;
    void removeKeyUnlocked(const std::string& key);
    static bool globMatch(const std::string& str, const std::string& pattern);

private:
    mutable std::mutex db_mutex;

    // Data stores
    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;

    // Expiries (not persisted): absolute deadlines (steady_clock)
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiries;
};

#endif // DATABASE_H
