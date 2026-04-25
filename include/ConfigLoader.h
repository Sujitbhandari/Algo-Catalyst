#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <stdexcept>

namespace AlgoCatalyst {

/**
 * Minimal JSON config loader - handles flat key/value pairs only.
 * Parses a subset of JSON: {"key": value, ...} where value is number or string.
 * No external dependencies; written for the project's simple config use-case.
 */
class ConfigLoader {
public:
    static ConfigLoader fromFile(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Cannot open config: " + path);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ConfigLoader(ss.str());
    }

    explicit ConfigLoader(const std::string& json) { parse(json); }

    bool has(const std::string& key) const {
        return values_.count(key) > 0;
    }

    double getDouble(const std::string& key, double default_val = 0.0) const {
        auto it = values_.find(key);
        if (it == values_.end()) return default_val;
        try { return std::stod(it->second); } catch (...) { return default_val; }
    }

    int getInt(const std::string& key, int default_val = 0) const {
        auto it = values_.find(key);
        if (it == values_.end()) return default_val;
        try { return std::stoi(it->second); } catch (...) { return default_val; }
    }

    std::string getString(const std::string& key, const std::string& default_val = "") const {
        auto it = values_.find(key);
        if (it == values_.end()) return default_val;
        return it->second;
    }

    bool getBool(const std::string& key, bool default_val = false) const {
        auto it = values_.find(key);
        if (it == values_.end()) return default_val;
        return it->second == "true" || it->second == "1";
    }

private:
    std::map<std::string, std::string> values_;

    static std::string trim(const std::string& s) {
        std::size_t start = s.find_first_not_of(" \t\r\n\"");
        std::size_t end   = s.find_last_not_of(" \t\r\n\"");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }

    void parse(const std::string& json) {
        std::string s = json;
        // Remove outer braces
        std::size_t lb = s.find('{');
        std::size_t rb = s.rfind('}');
        if (lb == std::string::npos || rb == std::string::npos) return;
        s = s.substr(lb + 1, rb - lb - 1);

        // Split by commas (simple, no nested objects)
        std::size_t pos = 0;
        while (pos < s.size()) {
            std::size_t comma = findNextComma(s, pos);
            std::string token = s.substr(pos, comma - pos);
            pos = (comma == std::string::npos) ? s.size() : comma + 1;

            std::size_t colon = token.find(':');
            if (colon == std::string::npos) continue;

            std::string key = trim(token.substr(0, colon));
            std::string val = trim(token.substr(colon + 1));

            // Strip inline comments
            std::size_t comment = val.find("//");
            if (comment != std::string::npos) val = trim(val.substr(0, comment));

            if (!key.empty() && !val.empty()) {
                values_[key] = val;
            }
        }
    }

    static std::size_t findNextComma(const std::string& s, std::size_t start) {
        int depth = 0;
        bool in_str = false;
        for (std::size_t i = start; i < s.size(); ++i) {
            char c = s[i];
            if (c == '"' && (i == 0 || s[i-1] != '\\')) in_str = !in_str;
            if (!in_str) {
                if (c == '{' || c == '[') depth++;
                if (c == '}' || c == ']') depth--;
                if (c == ',' && depth == 0) return i;
            }
        }
        return std::string::npos;
    }
};

} // namespace AlgoCatalyst
