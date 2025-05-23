#ifndef APPS_HTTPD_HTTPD_HH_
#define APPS_HTTPD_HTTPD_HH_

#include "request_parser.hh"
#include "request.hh"
#include "../future/future_all12.hh"
#include <string_view>
#include "../app/app-template.hh"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <string_view>
#include <queue>
#include <bitset>
#include <limits>
#include <cctype>
#include <vector>
#include <boost/intrusive/list.hpp>
#include "reply.hh"
#include "routes.hh"

namespace httpd {

class http_server;
class http_stats;

using namespace std::chrono_literals;

class http_stats {
    //seastar::metrics::metric_groups _metric_groups;
    public:
    http_stats(http_server& server, const std::string& name);
};

class http_server {
    std::vector<server_socket> _listeners;
    http_stats _stats;
    uint64_t _total_connections = 0;
    uint64_t _current_connections = 0;
    uint64_t _requests_served = 0;
    uint64_t _connections_being_accepted = 0;
    uint64_t _read_errors = 0;
    uint64_t _respond_errors = 0;
    std::string _date = http_date();
    timer<> _date_format_timer { [this] {_date = http_date();} };
    bool _stopping = false;
    promise<> _all_connections_stopped;
    future<> _stopped = _all_connections_stopped.get_future();
/*................待定.....*/
    http_server() : _stats(*this, "default_server") { // 提供一个默认名称
        _date_format_timer.arm_periodic(1s);
    }
/*.............................*/


private:
    void maybe_idle() {
        if (_stopping && !_connections_being_accepted && !_current_connections) {
            _all_connections_stopped.set_value();
        }
    }
public:
    routes _routes;

    explicit http_server(const std::string& name) : _stats(*this, name) {
        _date_format_timer.arm_periodic(1s);
    }
    future<> listen(ipv4_addr addr) {
        listen_options lo;
        lo.reuse_address = true;
        _listeners.push_back(engine().listen(make_ipv4_address(addr), lo));
        _stopped = when_all(std::move(_stopped), do_accepts(_listeners.size() - 1)).discard_result();
        return make_ready_future<>();
    }
    future<> stop() {
        _stopping = true;
        for (auto&& l : _listeners) {
            l.abort_accept();
        }
        for (auto&& c : _connections) {
            c.shutdown();
        }
        return std::move(_stopped);
    }
    future<> do_accepts(int which) {
        ++_connections_being_accepted;
        return _listeners[which].accept().then_wrapped(
                [this, which] (future<connected_socket, socket_address> f_cs_sa) mutable {
                    --_connections_being_accepted;
                    if (_stopping) {
                        maybe_idle();
                        return;
                    }
                    auto cs_sa = f_cs_sa.get();
                    auto conn = new connection(*this, std::get<0>(std::move(cs_sa)), std::get<1>(std::move(cs_sa)));
                    conn->process().then_wrapped([conn] (auto&& f) {
                                delete conn;
                                try {
                                    f.get();
                                } catch (std::exception& ex) {
                                    std::cerr << "request error " << ex.what() << std::endl;
                                }
                            });
                    do_accepts(which);
                }).then_wrapped([] (auto f){
            try {
                f.get();
            } catch (std::exception& ex) {
                std::cerr << "accept failed: " << ex.what() << std::endl;
            }
        });
    }
    class connection : public boost::intrusive::list_base_hook<> {
        http_server& _server;
        connected_socket _fd;
        input_stream<char> _read_buf;
        output_stream<char> _write_buf;
        static constexpr size_t limit = 4096;
        using tmp_buf = temporary_buffer<char>;
        http_request_parser _parser;
        std::unique_ptr<request> _req;
        std::unique_ptr<reply> _resp;
        // null element marks eof
        queue_<std::unique_ptr<reply>> _replies { 10 };
        bool _done = false;
    public:
        connection(http_server& server, connected_socket&& fd,
                socket_address addr)
                : _server(server), _fd(std::move(fd)), _read_buf(_fd.input()), _write_buf(
                        _fd.output()) {
            ++_server._total_connections;
            ++_server._current_connections;
            _server._connections.push_back(*this);
        }
        ~connection() {
            --_server._current_connections;
            _server._connections.erase(_server._connections.iterator_to(*this));
            _server.maybe_idle();
        }
        future<> process() {
            // Launch read and write "threads" simultaneously:
            return when_all(read(), respond()).then(
                    [] (std::tuple<future<>, future<>> joined) {
                        // FIXME: notify any exceptions in joined?
                        return make_ready_future<>();
                    });
        }
        void shutdown() {
            _fd.shutdown_input();
            _fd.shutdown_output();
        }
        future<> read() {
            return do_until([this] {return _done;}, [this] {
                return read_one();
            }).then_wrapped([this] (future<> f) {
                // swallow error
                if (f.failed()) {
                    _server._read_errors++;
                }
                f.ignore_ready_future();
                return _replies.push_eventually( {});
            }).finally([this] {
                return _read_buf.close();
            });
        }
        future<> read_one() {
            _parser.init();
            return _read_buf.consume(_parser).then([this] () mutable {
                if (_parser.eof()) {
                    _done = true;
                    return make_ready_future<>();
                }
                ++_server._requests_served;
                std::unique_ptr<httpd::request> req = _parser.get_parsed_request();
                return _replies.not_full().then([req = std::move(req), this] () mutable {
                    return generate_reply(std::move(req));
                }).then([this](bool done) {
                    _done = done;
                });
            });
        }
        future<> respond() {
            return do_response_loop().then_wrapped([this] (future<> f) {
                // swallow error
                if (f.failed()) {
                    _server._respond_errors++;
                }
                f.ignore_ready_future();
                return _write_buf.close();
            });
        }
        future<> do_response_loop() {
            return _replies.pop_eventually().then(
                    [this] (std::unique_ptr<reply> resp) {
                        if (!resp) {
                            // eof
                            return make_ready_future<>();
                        }
                        _resp = std::move(resp);
                        return start_response().then([this] {
                                    return do_response_loop();
                                });
                    });
        }
        future<> start_response() {
            _resp->_headers["Server"] = "Seastar httpd";
            _resp->_headers["Date"] = _server._date;
            _resp->_headers["Content-Length"] = std::to_string(
                    _resp->_content.size());
            return _write_buf.write(_resp->_response_line).then([this] {
                return write_reply_headers(_resp->_headers.begin());
            }).then([this] {
                return _write_buf.write("\r\n", 2);
            }).then([this] {
                return write_body();
            }).then([this] {
                return _write_buf.flush();
            }).then([this] {
                _resp.reset();
            });
        }
        future<> write_reply_headers(
                std::unordered_map<std::string, std::string>::iterator hi) {
            if (hi == _resp->_headers.end()) {
                return make_ready_future<>();
            }
            return _write_buf.write(hi->first).then(
                    [this] {
                        return _write_buf.write(": ", 2);
                    }).then([hi, this] {
                return _write_buf.write(hi->second);
            }).then([this] {
                return _write_buf.write("\r\n", 2);
            }).then([hi, this] () mutable {
                return write_reply_headers(++hi);
            });
        }
        static short hex_to_byte(char c) {
            if (c >='a' && c <= 'z') {
                return c - 'a' + 10;
            } else if (c >='A' && c <= 'Z') {
                return c - 'A' + 10;
            }
            return c - '0';
        }
        /**
         * Convert a hex encoded 2 bytes substring to char
         */
        static char hexstr_to_char(const std::string_view& in, size_t from) {

            return static_cast<char>(hex_to_byte(in[from]) * 16 + hex_to_byte(in[from + 1]));
        }
        /**
         * URL_decode a substring and place it in the given out std::string
         */
        static bool url_decode(const std::string_view& in, std::string& out) {
            size_t pos = 0;
            char buff[in.length()];
            for (size_t i = 0; i < in.length(); ++i) {
                if (in[i] == '%') {
                    if (i + 3 <= in.size()) {
                        buff[pos++] = hexstr_to_char(in, i + 1);
                        i += 2;
                    } else {
                        return false;
                    }
                } else if (in[i] == '+') {
                    buff[pos++] = ' ';
                } else {
                    buff[pos++] = in[i];
                }
            }
            out = std::string(buff, pos);
            return true;
        }
        /**
         * Add a single query parameter to the parameter list
         */
        static void add_param(request& req, const std::string_view& param) {
            size_t split = param.find('=');
            if (split >= param.length() - 1) {
                std::string key;
                if (url_decode(param.substr(0,split) , key)) {
                    req.query_parameters[key] = "";
                }
            }
            else
            {
                std::string key;
                std::string value;
                if (url_decode(param.substr(0,split), key)
                        && url_decode(param.substr(split + 1), value)) {
                    req.query_parameters[key] = value;
                }
            }
        }
        /**
         * Set the query parameters in the request objects.
         * query param appear after the question mark and are separated.
         * by the ampersand sign.
         */
        static std::string set_query_param(request& req) {
            size_t pos = req._url.find('?');
            if (pos == std::string::npos) {
                return req._url;
            }
            size_t curr = pos + 1;
            size_t end_param;
            std::string_view url = req._url;
            while ((end_param = req._url.find('&', curr)) != std::string::npos) {
                add_param(req, url.substr(curr, end_param - curr) );
                curr = end_param + 1;
            }
            add_param(req, url.substr(curr));
            return req._url.substr(0, pos);
        }

        future<bool> generate_reply(std::unique_ptr<request> req) {
            auto resp = std::make_unique<reply>();
            bool conn_keep_alive = false;
            bool conn_close = false;
            auto it = req->_headers.find("Connection");
            if (it != req->_headers.end()) {
                if (it->second == "Keep-Alive") {
                    conn_keep_alive = true;
                } else if (it->second == "Close") {
                    conn_close = true;
                }
            }
            bool should_close;
            // TODO: Handle HTTP/2.0 when it releases
            resp->set_version(req->_version);
            if (req->_version == "1.0") {
                if (conn_keep_alive) {
                    resp->_headers["Connection"] = "Keep-Alive";
                }
                should_close = !conn_keep_alive;
            } else if (req->_version == "1.1") {
                should_close = conn_close;
            } else {
                // HTTP/0.9 goes here
                should_close = true;
            }
            std::string url = set_query_param(*req.get());
            std::string version = req->_version;
            return _server._routes.handle(url, std::move(req), std::move(resp)).
            // Caller guarantees enough room
            then([this, should_close, version = std::move(version)](std::unique_ptr<reply> rep) {
                rep->set_version(version).done();
                this->_replies.push(std::move(rep));
                return make_ready_future<bool>(should_close);
            });
        }
        future<> write_body() {
            return _write_buf.write(_resp->_content);
        }
    };
    uint64_t total_connections() const {
        return _total_connections;
    }
    uint64_t current_connections() const {
        return _current_connections;
    }
    uint64_t requests_served() const {
        return _requests_served;
    }
    uint64_t read_errors() const {
        return _read_errors;
    }
    uint64_t reply_errors() const {
        return _respond_errors;
    }
    static std::string http_date() {
        auto t = ::time(nullptr);
        struct tm tm;
        gmtime_r(&t, &tm);
        char tmp[100];
        strftime(tmp, sizeof(tmp), "%d %b %Y %H:%M:%S GMT", &tm);
        return tmp;
    }
private:
    boost::intrusive::list<connection> _connections;
};

/*
 *  A helper class to start, set and listen an http server
 *  typical use would be:
 *  auto server =
 *          new http_server_control();
 *                server->start().then([server] {
 *                 server->set_routes(set_routes);
 *          }).then([server, port] {
 *                  server->listen(port);
 *          }).then([port] {
 *                  std::cout << "Seastar HTTP server listening on port " << port << " ...\n";
 *          });
 */


class http_server_control {
    distributed<http_server>* _server_dist;
private:
    static std::string generate_server_name();
public:
    http_server_control():_server_dist(new distributed<http_server>()){}

    future<> start(const std::string& name = generate_server_name()) {
        return _server_dist->start(name);
    }

    future<> stop() {
        return _server_dist->stop();
    }

    future<> set_routes(std::function<void(routes& r)> fun) {
        return _server_dist->invoke_on_all([fun](http_server& server) {
            fun(server._routes);
        });
    }

    future<> listen(ipv4_addr addr) {
        return _server_dist->invoke_on_all(&http_server::listen, addr);
    }

    distributed<http_server>& server() {
        return *_server_dist;
    }
};

}

using namespace std::chrono_literals;

namespace httpd {
http_stats::http_stats(http_server& server, const std::string& name)
{
    // namespace sm = seastar::metrics;
    // std::vector<sm::label_instance> labels;
    // labels.push_back(sm::label_instance("service", name));
}

std::string http_server_control::generate_server_name() {
    static thread_local uint16_t idgen;
    return "http-{}"+std::to_string(idgen++);
}
}

#endif /* APPS_HTTPD_HTTPD_HH_ */
