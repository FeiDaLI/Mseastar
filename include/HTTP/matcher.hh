#ifndef MATCHER_HH_
#define MATCHER_HH_

#include "common.hh"
#include <string>

namespace httpd {

/** a base class for the url matching. Each implementation check if the given url matches a criteria */

class matcher {
public:
    virtual ~matcher() = default;

    /**
     * check if the given url matches the rule
     * @param url the url to check
     * @param ind the position to start from
     * @param fill the parameters hash
     * @return the end of of the matched part, or std::string::npos if not matched
     */
    virtual size_t match(const std::string& url, size_t ind, parameters& param) = 0;
};

/**
 * Check if the url match a parameter and fill the parameters object
 *
 * Note that a non empty url will always return true with the parameters
 * object filled
 *
 * Assume that the rule is /file/{path}/ and the param_matcher identify
 * the /{path}
 *
 * For all non empty values, match will return true.
 * If the entire url is /file/etc/hosts, and the part that is passed to
 * param_matcher is /etc/hosts, if entire_path is true, the match will be
 * '/etc/hosts' If entire_path is false, the match will be '/etc'
 */
class param_matcher : public matcher {
public:
    /**
     * Constructor
     * @param name the name of the parameter, will be used as the key
     * in the parameters object
     * @param entire_path when set to true, the matched parameters will
     * include all the remaining url until the end of it.
     * when set to false the match will terminate at the next slash
     */
    explicit param_matcher(const std::string& name, bool entire_path = false)
            : _name(name), _entire_path(entire_path) {
    }

    virtual size_t match(const std::string& url, size_t ind, parameters& param)
            override;
private:
    std::string _name;
    bool _entire_path;
};

/**
 * Check if the url match a predefine string.
 *
 * When parsing a match rule such as '/file/{path}' the str_match would parse
 * the '/file' part
 */
class str_matcher : public matcher {
public:
    /**
     * Constructor
     * @param cmp the string to match
     */
    explicit str_matcher(const std::string& cmp)
            : _cmp(cmp), _len(cmp.size()) {
    }

    virtual size_t match(const std::string& url, size_t ind, parameters& param)
            override;
private:
    std::string _cmp;
    unsigned _len;
};
}

namespace httpd {

using namespace std;
static size_t find_end_param(const std::string& url, size_t ind, bool entire_path) {
    size_t pos = (entire_path) ? url.length() : url.find('/', ind + 1);
    if (pos == std::string::npos) {
        return url.length();
    }
    return pos;
}

size_t param_matcher::match(const std::string& url, size_t ind, parameters& param) {
    size_t last = find_end_param(url, ind, _entire_path);
    if (last == ind) {
        /*
         * empty parameter allows only for the case of entire_path
         */
        if (_entire_path) {
            param.set(_name, "");
            return ind;
        }
        return std::string::npos;
    }
    param.set(_name, url.substr(ind, last - ind));
    return last;
}

size_t str_matcher::match(const std::string& url, size_t ind, parameters& param) {
    if (url.length() >= _len + ind && (url.find(_cmp, ind) == ind)
            && (url.length() == _len + ind || url.at(_len + ind) == '/')) {
        return _len + ind;
    }
    return std::string::npos;
}

}


#endif /* MATCHER_HH_ */
