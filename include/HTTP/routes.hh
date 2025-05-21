#ifndef ROUTES_HH_
#define ROUTES_HH_

#include "matchrules.hh"
#include "handlers.hh"
#include "common.hh"
#include "reply.hh"
#include <boost/program_options/variables_map.hpp>
#include <unordered_map>
#include <boost/json.hpp>
#include <boost/json/serialize.hpp>
#include <vector>
#include "../../include/future/future_all12.hh"
#include "exception.hh"

namespace httpd {
    /** The url helps defining a route. */
class url {
public:
    /**
     * Move constructor
     */
    url(url&&) = default;

    /**
     * Construct with a url path as it's parameter
     * @param path the url path to be used
     */
    url(const std::string& path)
            : _path(path) {
    }
    /**
     * Adds a parameter that matches untill the end of the URL.
     * @param param the parmaeter name
     * @return the current url
     */
    url& remainder(const std::string& param) {
        this->_param = param;
        return *this;
    }

    std::string _path;
    std::string _param;
};

/**
 * routes object do the request dispatching according to the url.
 * It uses two decision mechanism exact match, if a url matches exactly
 * (an optional leading slash is permitted) it is choosen
 * If not, the matching rules are used.
 * matching rules are evaluated by their insertion order
 */
class routes {
public:
    /**
     * The destructor deletes the match rules and handlers
     */
    ~routes();

    /**
     * adding a handler as an exact match
     * @param url the url to match (note that url should start with /)
     * @param handler the desire handler
     * @return it self
     */
    routes& put(operation_type type, const std::string& url,
            handler_base* handler) {
        //FIXME if a handler is already exists, it need to be
        // deleted to prevent memory leak
        _map[type][url] = handler;
        return *this;
    }

    /**
     * add a rule to be used.
     * rules are search only if an exact match was not found.
     * rules are search by the order they were added.
     * First in higher priority
     * @param rule a rule to add
     * @param type the operation type
     * @return it self
     */
    routes& add(match_rule* rule, operation_type type = GET) {
        _rules[type].push_back(rule);
        return *this;
    }

    /**
     * Add a url match to a handler:
     * Example  routes.add(GET, url("/api").remainder("path"), handler);
     * @param type
     * @param url
     * @param handler
     * @return
     */
    routes& add(operation_type type, const url& url, handler_base* handler);

    /**
     * the main entry point.
     * the general handler calls this method with the request
     * the method takes the headers from the request and find the
     * right handler.
     * It then call the handler with the parameters (if they exists) found in the url
     * @param path the url path found
     * @param req the http request
     * @param rep the http reply
     */
    future<std::unique_ptr<reply> > handle(const std::string& path, std::unique_ptr<request> req, std::unique_ptr<reply> rep);

    /**
     * Search and return an exact match
     * @param url the request url
     * @return the handler if exists or nullptr if it does not
     */
    handler_base* get_exact_match(operation_type type, const std::string& url) {
        return (_map[type].find(url) == _map[type].end()) ?
                nullptr : _map[type][url];
    }

private:

    /**
     * Search and return a handler by the operation type and url
     * @param type the http operation type
     * @param url the request url
     * @param params a parameter object that will be filled during the match
     * @return a handler based on the type/url match
     */
    handler_base* get_handler(operation_type type, const std::string& url,
            parameters& params);

    /**
     * Normalize the url to remove the last / if exists
     * and get the parameter part
     * @param url the full url path
     * @param param_part will hold the string with the parameters
     * @return the url from the request without the last /
     */
    std::string normalize_url(const std::string& url);

    std::unordered_map<std::string, handler_base*> _map[NUM_OPERATION];
    std::vector<match_rule*> _rules[NUM_OPERATION];
public:
    using exception_handler_fun = std::function<std::unique_ptr<reply>(std::exception_ptr eptr)>;
    using exception_handler_id = size_t;
private:
    std::map<exception_handler_id, exception_handler_fun> _exceptions;
    exception_handler_id _exception_id = 0;
    // for optimization reason, the lambda function
    // that calls the exception_reply of the current object
    // is stored
    exception_handler_fun _general_handler;
public:
    /**
     * The exception_handler_fun expect to call
     * std::rethrow_exception(eptr);
     * and catch only the exception it handles
     */
    exception_handler_id register_exeption_handler(exception_handler_fun fun) {
        auto current = _exception_id++;
        _exceptions[current] = fun;
        return current;
    }

    void remove_exception_handler(exception_handler_id id) {
        _exceptions.erase(id);
    }

    std::unique_ptr<reply> exception_reply(std::exception_ptr eptr);

    routes();
};

/**
 * A helper function that check if a parameter is found in the params object
 * if it does not the function would throw a parameter not found exception
 * @param params the parameters object
 * @param param the parameter to look for
 */
void verify_param(const httpd::request& req, const std::string& param);

}




namespace httpd {

using namespace std;

void verify_param(const request& req, const std::string& param) {
    if (req.get_query_param(param) == "") {
        throw missing_param_exception(param);
    }
}
routes::routes() : _general_handler([this](std::exception_ptr eptr) mutable {
    return exception_reply(eptr);
}) {}

routes::~routes() {
    for (int i = 0; i < NUM_OPERATION; i++) {
        for (auto kv : _map[i]) {
            delete kv.second;
        }
    }
    for (int i = 0; i < NUM_OPERATION; i++) {
        for (auto r : _rules[i]) {
            delete r;
        }
    }
}

std::unique_ptr<reply> routes::exception_reply(std::exception_ptr eptr) {
    auto rep = std::make_unique<reply>();
    try {
        for (auto e : _exceptions) {
            try {
                return e.second(eptr);
            } catch (...) {
                eptr = std::current_exception();
            }
        }
        std::rethrow_exception(eptr);
    } catch (const base_exception& e) {
        rep->set_status(e.status(), boost::json::serialize(json_exception(e).to_json()));
    } catch (exception& e) {
        rep->set_status(reply::status_type::internal_server_error,
                        boost::json::serialize(json_exception(e).to_json()));
    } catch (...) {
        rep->set_status(reply::status_type::internal_server_error,
                        boost::json::serialize(json_exception(std::runtime_error("Unknown unhandled exception")).to_json()));
    }
    rep->done("json");
    return rep;
}

future<std::unique_ptr<reply>> routes::handle(const std::string& path, std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
    handler_base* handler = get_handler(str2type(req->_method),
            normalize_url(path), req->param);
    if (handler != nullptr) {
        try {
            for (auto& i : handler->_mandatory_param) {
                verify_param(*req.get(), i);
            }
            auto r = handler->handle(path, std::move(req), std::move(rep));
            return r.handle_exception(_general_handler);
        } catch (const redirect_exception& _e) {
            rep.reset(new reply());
            rep->add_header("Location", _e.url).set_status(_e.status()).done("json");
        } catch (...) {
            rep = exception_reply(std::current_exception());
        }
    } else {
        rep.reset(new reply());
        json_exception ex(not_found_exception("Not found"));
        rep->set_status(reply::status_type::not_found, boost::json::serialize(ex.to_json())).done("json");
    }
    return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
}
std::string routes::normalize_url(const std::string& url) {
    if (url.length() < 2 || url.at(url.length() - 1) != '/') {
        return url;
    }
    return url.substr(0, url.length() - 1);
}

handler_base* routes::get_handler(operation_type type, const std::string& url,
        parameters& params) {
    handler_base* handler = get_exact_match(type, url);
    if (handler != nullptr) {
        return handler;
    }

    for (auto rule = _rules[type].cbegin(); rule != _rules[type].cend();
            ++rule) {
        handler = (*rule)->get(url, params);
        if (handler != nullptr) {
            return handler;
        }
        params.clear();
    }
    return nullptr;
}

routes& routes::add(operation_type type, const url& url,
        handler_base* handler) {
    match_rule* rule = new match_rule(handler);
    rule->add_str(url._path);
    if (url._param != "") {
        rule->add_param(url._param, true);
    }
    return add(rule, type);
}

}


#endif /* ROUTES_HH_ */
