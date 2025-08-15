#include "Database.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

using ClockType = std::chrono::steady_clock;

// Singleton
Database& Database::getInstance() {
    static Database instance;
    return instance;
}

// --- internal helpers (mutex must be held) ---
bool Database::keyExistsUnlocked(const std::string& key) const {
    return kv_store.find(key) != kv_store.end()
        || list_store.find(key) != list_store.end()
        || hash_store.find(key) != hash_store.end();
}

bool Database::isExpiredUnlocked(const std::string& key,
                                 const ClockType::time_point& now) const {
    auto it = expiries.find(key);
    if (it == expiries.end()) return false;
    return now >= it->second;
}

void Database::removeKeyUnlocked(const std::string& key) {
    kv_store.erase(key);
    list_store.erase(key);
    hash_store.erase(key);
    expiries.erase(key);
}

// Very small glob matcher: supports '*' and '?'
bool Database::globMatch(const std::string& str, const std::string& pat) {
    // iterative backtracking
    size_t s = 0, p = 0, star = std::string::npos, ss = 0;
    while (s < str.size()) {
        if (p < pat.size() && (pat[p] == '?' || pat[p] == str[s])) { ++s; ++p; }
        else if (p < pat.size() && pat[p] == '*') { star = p++; ss = s; }
        else if (star != std::string::npos) { p = star + 1; s = ++ss; }
        else return false;
    }
    while (p < pat.size() && pat[p] == '*') ++p;
    return p == pat.size();
}

// Opportunistic purge on access, and a callable for background thread
void Database::purgeExpired() {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    std::vector<std::string> toErase;
    toErase.reserve(expiries.size());
    for (auto &kv : expiries) {
        if (now >= kv.second) toErase.push_back(kv.first);
    }
    for (auto &k : toErase) removeKeyUnlocked(k);
}

// ---------- STRING OPS ----------
bool Database::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        removeKeyUnlocked(key);
    }
    kv_store[key] = value;
    return true;
}

std::optional<std::string> Database::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        removeKeyUnlocked(key);
        return std::nullopt;
    }
    auto it = kv_store.find(key);
    if (it != kv_store.end()) return it->second;
    return std::nullopt;
}

bool Database::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        removeKeyUnlocked(key);
        return false;
    }
    bool removed = false;
    if (kv_store.erase(key)) removed = true;
    if (list_store.erase(key)) removed = true;
    if (hash_store.erase(key)) removed = true;
    expiries.erase(key);
    return removed;
}

long Database::incr(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        removeKeyUnlocked(key);
    }

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

bool Database::exists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        // Note: we cannot modify maps in const method; treat as not existing.
        return false;
    }
    return keyExistsUnlocked(key);
}

// ---------- LIST OPS ----------
size_t Database::lpush(const std::string& key, const std::vector<std::string>& values) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        removeKeyUnlocked(key);
    }
    auto &lst = list_store[key];
    lst.insert(lst.begin(), values.begin(), values.end());
    return lst.size();
}

std::optional<std::string> Database::lpop(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        removeKeyUnlocked(key);
        return std::nullopt;
    }
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        std::string val = it->second.front();
        it->second.erase(it->second.begin());
        return val;
    }
    return std::nullopt;
}

std::vector<std::string> Database::lrange(const std::string& key, int start, int stop) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        removeKeyUnlocked(key);
        return {};
    }
    auto it = list_store.find(key);
    if (it == list_store.end()) return {};

    const auto &lst = it->second;
    int n = static_cast<int>(lst.size());
    auto norm = [n](int idx) {
        if (idx < 0) idx = n + idx;
        if (idx < 0) idx = 0;
        if (idx >= n) idx = n - 1;
        return idx;
    };
    if (n == 0) return {};
    start = norm(start);
    stop  = norm(stop);
    if (stop < start) return {};

    std::vector<std::string> out;
    out.reserve(static_cast<size_t>(stop - start + 1));
    for (int i = start; i <= stop && i < n; ++i) {
        out.push_back(lst[static_cast<size_t>(i)]);
    }
    return out;
}

// ---------- Expiry & key management ----------
bool Database::expire(const std::string& key, int seconds) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        removeKeyUnlocked(key);
        return false;
    }
    if (!keyExistsUnlocked(key)) return false;
    expiries[key] = now + std::chrono::seconds(seconds);
    return true;
}

long Database::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();
    if (isExpiredUnlocked(key, now)) {
        removeKeyUnlocked(key);
        return -2; // key no longer exists
    }
    if (!keyExistsUnlocked(key)) return -2;
    auto it = expiries.find(key);
    if (it == expiries.end()) return -1;
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(it->second - now).count();
    if (diff < 0) diff = 0;
    return static_cast<long>(diff);
}

std::vector<std::string> Database::keys(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(db_mutex);
    const auto now = ClockType::now();

    std::vector<std::string> out;
    out.reserve(kv_store.size() + list_store.size() + hash_store.size());

    auto pushIfAlive = [&](const std::string& k) {
        if (isExpiredUnlocked(k, now)) return;
        if (globMatch(k, pattern)) out.push_back(k);
    };

    for (auto &kv : kv_store) pushIfAlive(kv.first);
    for (auto &kv : list_store) pushIfAlive(kv.first);
    for (auto &kv : hash_store) pushIfAlive(kv.first);

    return out;
}

// ---------- Persistence (simple text) ----------
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
    expiries.clear();

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
