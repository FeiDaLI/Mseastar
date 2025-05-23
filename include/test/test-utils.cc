
// hack: define it even when statically linking, to avoid Boost.Test defining main()
#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif

#include <thread>
#include "test-utils.hh"
#include <boost/test/included/unit_test.hpp>

void seastar_test::run() {
    // HACK: please see https://github.com/cloudius-systems/seastar/issues/10
    BOOST_REQUIRE(true);

    // HACK: please see https://github.com/cloudius-systems/seastar/issues/10
    boost::program_options::variables_map()["dummy"];

    global_test_runner().run_sync([this] {
        return run_test_case();
    });
}

// We store a pointer because tests are registered from dynamic initializers,
// so we must ensure that 'tests' is initialized before any dynamic initializer.
// I use a primitive type, which is guaranteed to be initialized before any
// dynamic initializer and lazily allocate the factor.

static std::vector<seastar_test*>* tests;

seastar_test::seastar_test() {
    if (!tests) {
        tests = new std::vector<seastar_test*>();
    }
    tests->push_back(this);
}

bool init_unit_test_suite() {
    auto&& ts = boost::unit_test::framework::master_test_suite();
    ts.p_name.set(tests->size() ? (*tests)[0]->get_test_file() : "seastar-tests");

    for (seastar_test* test : *tests) {
    #if BOOST_VERSION > 105800
        ts.add(boost::unit_test::make_test_case([test] { test->run(); }, test->get_name(),
                test->get_test_file(), 0), 0, 0);
    #else
        ts.add(boost::unit_test::make_test_case([test] { test->run(); }, test->get_name()), 0, 0);
    #endif
    }

    global_test_runner().start(ts.argc, ts.argv);
    return true;
}

int main(int ac, char** av) {
    return ::boost::unit_test::unit_test_main(&init_unit_test_suite, ac, av);
}
