#include <gtest/gtest.h>
#include "../include/task/task.hh"

// 测试 lambda_task 的 run 方法
TEST(LambdaTaskTest, RunTest) {
    int value = 0;
    auto func = [&value]() { value = 42; };
    lambda_task<decltype(func)> task(func);
    task.run();
    EXPECT_EQ(value, 42);
}

// 测试 make_task 函数
TEST(MakeTaskTest, MakeTaskTest) {
    int value = 0;
    auto func = [&value]() { value = 100; };
    auto task = make_task(func);
    task->run();
    EXPECT_EQ(value, 100);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}