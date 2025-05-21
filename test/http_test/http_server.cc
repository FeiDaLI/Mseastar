#include "../../include/HTTP/httpd.hh"
#include "../../include/HTTP/handlers.hh"
#include "../../include/HTTP/matcher.hh"
#include "../../include/HTTP/matchrules.hh"
#include "../../include/HTTP/routes.hh"
#include "../../include/HTTP/exception.hh"
#include "../../include/HTTP/transformers.hh"
#include "../../include/app/app-template.hh"
namespace bpo = boost::program_options;
class handl : public httpd::handler_base {
public:
    virtual future<std::unique_ptr<reply>> handle(const std::string& path,
            std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
        if (path == "/api") {
            rep->set_status(httpd::reply::status_type::ok, "");
            rep->done();
            return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
        }
        rep->done("html");
        return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
    }
};

int main(int argc, char** argv) {
    app_template app;
    return app.run(argc, argv, [] {
        auto server = new httpd::http_server_control();
        return server->start().then([server] {
            server->set_routes([](httpd::routes& r) {
                r.add(httpd::operation_type::GET, httpd::url("/api"), new handl());
                r.add(httpd::operation_type::GET, httpd::url("/"), new handl());
            });
            return server->listen(ipv4_addr("0.0.0.0", 8080));
        }).then([] {
            std::cout << "HTTP服务器监听于 0.0.0.0:8080 ..." << std::endl;
        });
    });
}