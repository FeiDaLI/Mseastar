/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright 2015 Cloudius Systems
 */

#ifndef FORMATTER_HH_
#define FORMATTER_HH_

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <time.h>
#include <sstream>
namespace json {

struct jsonable;

typedef struct tm date_time;

/**
 * The formatter prints json values in a json format
 * it overload to_json method for each of the supported format
 * all to_json parameters are passed as a pointer
 */
class formatter {
    enum class state {
        none, array, map
    };
    static std::string begin(state);
    static std::string end(state);

    template<typename K, typename V>
    static std::string to_json(state s, const std::pair<K, V>& p) {
        return s == state::array ?
                        "{" + to_json(state::none, p) + "}" :
                        to_json(p.first) + ":" + to_json(p.second);
    }

    template<typename Iter>
    static std::string to_json(state s, Iter i, Iter e) {
        std::stringstream res;
        res << begin(s);
        size_t n = 0;
        while (i != e) {
            if (n++ != 0) {
                res << ",";
            }
            res << to_json(s, *i++);
        }
        res << end(s);
        return res.str();
    }

    // fallback template
    template<typename T>
    static std::string to_json(state, const T& t) {
        return to_json(t);
    }
public:

    /**
     * return a json formated string
     * @param str the string to format
     * @return the given string in a json format
     */
    static std::string to_json(const std::string& str);

    /**
     * return a json formated int
     * @param n the int to format
     * @return the given int in a json format
     */
    static std::string to_json(int n);

    /**
     * return a json formated long
     * @param n the long to format
     * @return the given long in a json format
     */
    static std::string to_json(long n);

    /**
     * return a json formated float
     * @param n the float to format
     * @return the given float in a json format
     */
    static std::string to_json(float f);

    /**
     * return a json formated double
     * @param d the double to format
     * @return the given double in a json format
     */
    static std::string to_json(double d);

    /**
     * return a json formated char* (treated as string)
     * @param str the char* to foramt
     * @return the given char* in a json foramt
     */
    static std::string to_json(const char* str);

    /**
     * return a json formated bool
     * @param d the bool to format
     * @return the given bool in a json format
     */
    static std::string to_json(bool d);

    /**
     * return a json formated list of a given vector of params
     * @param vec the vector to format
     * @return the given vector in a json format
     */
    template<typename... Args>
    static std::string to_json(const std::vector<Args...>& vec) {
        return to_json(state::array, vec.begin(), vec.end());
    }

    template<typename... Args>
    static std::string to_json(const std::map<Args...>& map) {
        return to_json(state::map, map.begin(), map.end());
    }

    template<typename... Args>
    static std::string to_json(const std::unordered_map<Args...>& map) {
        return to_json(state::map, map.begin(), map.end());
    }

    /**
     * return a json formated date_time
     * @param d the date_time to format
     * @return the given date_time in a json format
     */
    static std::string to_json(const date_time& d);

    /**
     * return a json formated json object
     * @param obj the date_time to format
     * @return the given json object in a json format
     */
    static std::string to_json(const jsonable& obj);

    /**
     * return a json formated unsigned long
     * @param l unsigned long to format
     * @return the given unsigned long in a json format
     */
    static std::string to_json(unsigned long l);

private:

    static constexpr const char* TIME_FORMAT = "%a %b %d %I:%M:%S %Z %Y";

};

}
#endif /* FORMATTER_HH_ */
