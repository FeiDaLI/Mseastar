#include "../../include/HTTP/httpd.hh"
#include "../../include/HTTP/handlers.hh"
#include "../../include/HTTP/matcher.hh"
#include "../../include/HTTP/matchrules.hh"
#include "../../include/HTTP/routes.hh"
#include "../../include/HTTP/exception.hh"
#include "../../include/HTTP/transformers.hh"
#include "../../include/app/app-template.hh"
namespace bpo = boost::program_options;

using namespace httpd;

class handl : public httpd::handler_base {
public:
    virtual future<std::unique_ptr<reply> > handle(const std::string& path,
            std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
        rep->done("html");
        return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
    }
};

int main(int ac,char **av){
    app_template app;
    return app.run_deprecated(ac, av, [&] {
        handl* h1 = new handl();
        handl* h2 = new handl();
        routes route;
        std::cout<<"route add /api"<<std::endl;
        route.add(operation_type::GET, url("/api").remainder("path"), h1);
        route.add(operation_type::GET, url("/"), h2);
        std::unique_ptr<request> req = std::make_unique<request>();
        std::unique_ptr<reply> rep = std::make_unique<reply>();
        auto f1 = route.handle("/api", std::move(req), std::move(rep)).then(
                        [] (std::unique_ptr<reply> rep) {
                            if((int)rep->_status==(int)reply::status_type::ok){
                                std::cout << "ok" << std::endl;
                            }
                        });
        req.reset(new request);
        rep.reset(new reply);
        auto f2 = route.handle("/", std::move(req), std::move(rep)).then([] (std::unique_ptr<reply> rep) {
                            // BOOST_REQUIRE_EQUAL((int )rep->_status, (int )reply::status_type::ok);
                            if((int)rep->_status==(int)reply::status_type::ok){
                                std::cout << "ok###########" << std::endl;
                            }
                    });
        req.reset(new request);
        rep.reset(new reply);
        auto f3 =  route.handle("/api/abc", std::move(req), std::move(rep)).then(
                        [] (std::unique_ptr<reply> rep) {});
        req.reset(new request);
        rep.reset(new reply);
        auto f4 =  route.handle("/ap", std::move(req), std::move(rep)).then(
                        [] (std::unique_ptr<reply> rep) {
                            if((int)rep->_status==(int)reply::status_type::not_found){
                                std::cout << "not found"<<std::endl;
                            }
                        });
        auto f = when_all(std::move(f1), std::move(f2), std::move(f3), std::move(f4))
                .then([] (std::tuple<future<>, future<>, future<>, future<>> fs) {
            std::get<0>(fs).get();
            std::get<1>(fs).get();
            std::get<2>(fs).get();
            std::get<3>(fs).get();
        });
    });
    return 0;
}