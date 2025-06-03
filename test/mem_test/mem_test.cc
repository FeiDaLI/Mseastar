#include "../../include/app/app-template.hh"

using namespace net;
using namespace std::chrono_literals;
namespace bpo = boost::program_options;


future<> alloc_almost_all_and_realloc_it_with_a_smaller_size() {
    auto all = memory::stats().total_memory();
    std::cout << "Total memory available: " << all << " bytes" << std::endl;
    std::cout<<"hello world##"<<std::endl;
    auto reserve = size_t(0.02 * all);
    auto to_alloc = all - (reserve + (10 << 20));
    std::cout << "Reserve memory: " << reserve << " bytes" << std::endl;
    std::cout << "Memory to allocate: " << to_alloc << " bytes" << std::endl;
    std::cout << "Attempting to allocate " << to_alloc << " bytes..." << std::endl;
    auto obj = memory::allocate(to_alloc);
    // assert(obj != nullptr);
    // auto obj2 = memory::realloc(obj, to_alloc - (1 << 20));
    // assert(obj == obj2);
    // free(obj2);
    return make_ready_future<>();
}



int main(int ac, char ** av) {
    app_template app;
    return app.run_deprecated(ac, av, [&] {
        alloc_almost_all_and_realloc_it_with_a_smaller_size().then_wrapped([](auto &&f){
            f.get();
        });
    });
}
