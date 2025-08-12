#include "Database.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

// Singleton
Database& Database::getInstance() {
    static Database instance;
    return instance;
}

// ---------- STRING OPS ----------
bool Database::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store[key] = value;
    return true;
}

std::optional<std::string> Database::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = kv_store.find(key);
    if (it != kv_store.end()) return it->second;
    return std::nullopt;
}

bool Database::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    bool removed = false;
    if (kv_store.erase(key)) removed = true;
    if (list_store.erase(key)) removed = true;
    if (hash_store.erase(key)) removed = true;
    return removed;
}

long Database::incr(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    long value = 0;
    auto it = kv_store.find(key);
    if (it != kv_store.end()) {
        try {
            value = std::stol(it->second);
        } catch (...) {
            throw std::runtime_error("value is not an integer");
        }
    }
    value++;
    kv_store[key] = std::to_string(value);
    return value;
}

// ---------- LIST OPS ----------
size_t Database::lpush(const std::string& key, const std::vector<std::string>& values) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto &lst = list_store[key];
    // insert at beginning in the same order values[0] becomes first element
    lst.insert(lst.begin(), values.begin(), values.end());
    return lst.size();
}

std::optional<std::string> Database::lpop(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        std::string val = it->second.front();
        it->second.erase(it->second.begin());
        return val;
    }
    return std::nullopt;
}

// ---------- PERSISTENCE ----------
bool Database::dump(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return false;

    for (const auto &kv : kv_store) {
        ofs << "K " << kv.first << " " << kv.second << "\n";
    }
    for (const auto &kv : list_store) {
        ofs << "L " << kv.first;
        for (const auto &item : kv.second) ofs << " " << item;
        ofs << "\n";
    }
    for (const auto &kv : hash_store) {
        ofs << "H " << kv.first;
        for (const auto &field_val : kv.second) {
            ofs << " " << field_val.first << ":" << field_val.second;
        }
        ofs << "\n";
    }
    return true;
}

bool Database::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return false;

    kv_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        char type;
        iss >> type;
        if (type == 'K') {
            std::string key, value;
            iss >> key >> value;
            kv_store[key] = value;
        } else if (type == 'L') {
            std::string key, item;
            iss >> key;
            std::vector<std::string> vec;
            while (iss >> item) vec.push_back(item);
            list_store[key] = std::move(vec);
        } else if (type == 'H') {
            std::string key, pair;
            iss >> key;
            std::unordered_map<std::string, std::string> map;
            while (iss >> pair) {
                auto pos = pair.find(':');
                if (pos != std::string::npos) {
                    map[pair.substr(0, pos)] = pair.substr(pos + 1);
                }
            }
            hash_store[key] = std::move(map);
        }
    }
    return true;
}
