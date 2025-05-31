#include "../../include/app/app-template.hh"
using namespace net;
using namespace std::chrono_literals;

namespace bpo = boost::program_options;

future<int> f(int x){
    return make_ready_future<int>(x+1);
}

int main(int ac, char ** av) {
    app_template app;
    return app.run_deprecated(ac, av, [&] {
        f(1).then([](int x){
            std::cout<<"then1"<<std::endl;
            return 11;
        }).then_wrapped([](auto&& f) {
            try {
                auto x = f.get();
                std::cout<<"x value"<<std::get<0>(x)<<std::endl;
                std::cout<<"then_wrapped 1"<<std::endl;
            } catch (...) {
                std::cout<<"then_wrapped 2"<<std::endl;
            }
        });
    });
}
