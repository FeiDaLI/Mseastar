#include <gtest/gtest.h>
#include "../include/future/future_state.hh"

struct TestStruct {
    int x;
    int y;
    TestStruct(int x, int y) : x(x), y(y) {};
    int getX(){return x;}
    int getY(){return y;}
};


TEST(FutureStateTest, CustomType) {
    future_state<TestStruct> state;
    TestStruct test_struct(1, 2);
    state.set(test_struct);
    auto v = std::get<0>(state.get_value()); //state.get_value是一个tuple
    EXPECT_EQ(v.getX(),1);
    EXPECT_EQ(v.getY(),2);
}




// 测试 future_state 的默认构造函数和状态
TEST(FutureStateTest, DefaultConstructor) {
    future_state<int> state;
    EXPECT_EQ(state.available(), false);
    EXPECT_EQ(state.failed(), false);
}

// 测试 future_state 的 set 和 get 方法
TEST(FutureStateTest, SetValueAndGet) {
    future_state<int> state;
    state.set(std::make_tuple(42));
    EXPECT_EQ(state.available(), true);
    EXPECT_EQ(std::get<0>(state.get_value()), 42);
}

// 测试 future_state 的异常设置和获取
TEST(FutureStateTest, SetExceptionAndGet) {
    future_state<int> state;
    auto ex = std::make_exception_ptr(std::runtime_error("test exception"));
    state.set_exception(ex);
    EXPECT_EQ(state.available(), true);
    EXPECT_EQ(state.failed(), true);
    EXPECT_THROW(std::rethrow_exception(std::move(state).get_exception()), std::runtime_error);
}

// 测试 future_state 的移动语义
TEST(FutureStateTest, MoveSemantics) {
    future_state<int,int> state1;
    state1.set(std::make_tuple(42,23));
    future_state<int,int> state2 = std::move(state1);
    EXPECT_EQ(state1.available(), false); // state1 应该无效
    EXPECT_EQ(state2.available(), true);  // state2 应该有效
    EXPECT_EQ(std::get<0>(std::move(state2).get()), 42);
}

// 测试 future_state 的 ignore 方法
TEST(FutureStateTest, IgnoreMethod) {
    future_state<int> state;
    state.set(std::make_tuple(42));
    state.ignore();
    EXPECT_EQ(state.available(), false);
}


//验证特化版本的future_states
TEST(FutureStateTest, test1) {
    future_state<> state;
    state.set();
    auto v = state.get();
    // 获取 tuple 的大小
    constexpr size_t tuple_size = std::tuple_size<decltype(v)>::value;
    // 验证大小是否为 0
    EXPECT_EQ(tuple_size, 0);
}