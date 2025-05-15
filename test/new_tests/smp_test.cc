// #include"../include/future/future_all8.hh"
#include"../include/app/app-template.hh"

future<bool> test_smp_call() {
    std::cout<<"开始执行test_smp_call函数"<<std::endl;
    return smp::submit_to(1, [] {
        return make_ready_future<int>(3);
    }).then([] (int ret) {
        return make_ready_future<bool>(ret == 3);
    });
}

struct nasty_exception {};
future<bool> test_smp_exception() {
    printf("1\n");
    return smp::submit_to(1, [] {
        printf("2\n");
        auto x = make_exception_future<int>(nasty_exception());
        printf("3\n");
        return x;
    }).then_wrapped([] (future<int> result) {
        printf("4\n");
        try {
            result.get();
            return make_ready_future<bool>(false); // expected an exception
        } catch (nasty_exception&) {
            // all is well
            return make_ready_future<bool>(true);
        } catch (...) {
            // incorrect exception type
            return make_ready_future<bool>(false);
        }
    });
}

int tests, fails;

future<> report(std::string msg, future<bool>&& result) {
    std::cout<<"开始执行report函数"<<std::endl;
    return std::move(result).then([msg] (bool result) {
        printf("%s: %s\n", (result ? "PASS" : "FAIL"), msg);
        tests += 1;
        fails += !result;
    });
}

int main(int ac, char** av) {
    return app_template().run_deprecated(ac, av, [] {
       return report("smp call", test_smp_call()).then([] {
           return report("smp exception", test_smp_exception());
       }).then([] {
           printf("\n %d tests / %d failures\n", tests, fails);
           engine().exit(fails ? 1 : 0);
       });
    });
}

// int main(int ac, char** av) {
//     return app_template().run_deprecated(ac, av, [] {
//         std::cout<<"开始执行main函数"<<std::endl;
//         return 0;
//     });
// }
