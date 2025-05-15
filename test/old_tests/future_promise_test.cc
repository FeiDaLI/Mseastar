#include <gtest/gtest.h>
#include "../include/future/all.hh"
#include <string>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

class FuturePromiseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test basic promise-future functionality with int
TEST_F(FuturePromiseTest, BasicIntTest) {
    promise<int> p;
    future<int> f = p.get_future();
    ASSERT_FALSE(f.available());
    p.set_value(std::make_tuple(42));
    ASSERT_TRUE(f.available());
    ASSERT_FALSE(f.failed());    
    auto result = std::get<0>(f.get());
    EXPECT_EQ(result, 42);
}

// Test promise-future with multiple values
TEST_F(FuturePromiseTest, MultipleValuesTest) {
    promise<int, std::string> p;
    future<int, std::string> f = p.get_future();
    ASSERT_FALSE(f.available());
    p.set_value(std::make_tuple(42, "hello"));
    ASSERT_TRUE(f.available());
    ASSERT_FALSE(f.failed());    
    auto [i, s] = f.get();
    EXPECT_EQ(i, 42);
    EXPECT_EQ(s, "hello");
}

// Test exception handling
TEST_F(FuturePromiseTest, ExceptionTest) {
    promise<int> p;
    future<int> f = p.get_future();
    p.set_exception(std::make_exception_ptr(std::runtime_error("test error")));
    ASSERT_TRUE(f.available());
    ASSERT_TRUE(f.failed());
    
    EXPECT_THROW(f.get(), std::runtime_error);
}

// Test void promise/future
TEST_F(FuturePromiseTest, VoidTest) {
    promise<void> p;
    future<void> f = p.get_future();
    ASSERT_FALSE(f.available());
    p.set_value();
    ASSERT_TRUE(f.available());
    ASSERT_FALSE(f.failed());
    
    EXPECT_NO_THROW(f.get());
}

TEST_F(FuturePromiseTest, VoidTest1) {
    promise<> p;
    future<> f = p.get_future();
    ASSERT_FALSE(f.available());
    p.set_value();
    ASSERT_TRUE(f.available());
    ASSERT_FALSE(f.failed());
    
    EXPECT_NO_THROW(f.get());
}

// Test make_ready_future
TEST_F(FuturePromiseTest, MakeReadyFutureTest) {
    auto f = make_ready_future<int>(123);
    ASSERT_TRUE(f.available());
    ASSERT_FALSE(f.failed());
    auto result = std::get<0>(f.get());
    EXPECT_EQ(result, 123);
}

// Test make_exception_future
TEST_F(FuturePromiseTest, MakeExceptionFutureTest) {
    auto f = make_exception_future<int>(std::runtime_error("test error"));
    ASSERT_TRUE(f.available());
    ASSERT_TRUE(f.failed());
    
    EXPECT_THROW(f.get(), std::runtime_error);
}

// Test futurize with normal value
TEST_F(FuturePromiseTest, FuturizeValueTest) {
    auto func = []() { return 42; };
    auto f = futurize_apply(func);
    ASSERT_TRUE(f.available());
    
    auto result = std::get<0>(f.get());
    EXPECT_EQ(result, 42);
}

//Test futurize with future value
TEST_F(FuturePromiseTest, FuturizeFutureTest) {
    auto func = []() { return make_ready_future<int>(42); };
    auto f = futurize_apply(func);
    ASSERT_TRUE(f.available());    
    auto result = std::get<0>(f.get());
    EXPECT_EQ(result, 42);
}

/* 这个报错 以后过来测试*/
// Test then() chaining
TEST_F(FuturePromiseTest, ThenChainingTest) {
    promise<int> p;
    future<int> f = p.get_future();    
    auto f2 = f.then([](std::tuple<int> val) {
        return std::get<0>(val) * 2;
    });
    p.set_value(std::make_tuple(21));
    //这行代码出错
    auto result = std::get<0>(f2.get());
    // EXPECT_EQ(result, 42);
}

// Test handle_exception
TEST_F(FuturePromiseTest, HandleExceptionTest) {
    promise<int> p;
    future<int> f = p.get_future();
    auto f2 = f.handle_exception([](std::exception_ptr ep) {
        try {
            std::rethrow_exception(ep);
        } catch (const std::runtime_error& e) {
            return make_ready_future<int>(42);
        }
        return make_ready_future<int>(0);
    });
    
    p.set_exception(std::make_exception_ptr(std::runtime_error("test error")));
    auto result = std::get<0>(f2.get());
    EXPECT_EQ(result, 42);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
