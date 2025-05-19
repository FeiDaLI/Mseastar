#ifndef COMMON_HH_
#define COMMON_HH_
#include <unordered_map>
#include <string>

namespace httpd {

class parameters {
    std::unordered_map<std::string, std::string> params;
public:
    const std::string& path(const std::string& key) const {
        return params.at(key);
    }
    std::string operator[](const std::string& key) const {
        return params.at(key).substr(1);
    }
    const std::string& at(const std::string& key) const {
        return path(key);
    }
    bool exists(const std::string& key) const {
        return params.find(key) != params.end();
    }
    void set(const std::string& key, const std::string& value) {
        params[key] = value;
    }
    void clear() {
        params.clear();
    }
};

enum operation_type {
    GET, POST, PUT, DELETE, NUM_OPERATION
};

/**
 * Translate the string command to operation type
 * @param type the string "GET" or "POST"
 * @return the operation_type
 */
operation_type str2type(const std::string& type);

}

#endif /* COMMON_HH_ */
