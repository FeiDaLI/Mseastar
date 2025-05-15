#include <gtest/gtest.h>
#include "../../include/future/expiring_fifo.hh" // Include the header for expiring_fifo
#include <chrono>
#include <thread>

using namespace std::chrono_literals; // For using literals like 1s

class ExpiringFifoTest : public ::testing::Test {
protected:
    using FifoType = expiring_fifo<int>; // Ensure this matches your expiring_fifo definition
    FifoType fifo;

    void SetUp() override {
        // Optional: Set up code before each test
    }

    void TearDown() override {
        // Optional: Clean up code after each test
    }
};

TEST_F(ExpiringFifoTest, TestEmpty) {
    EXPECT_TRUE(fifo.empty());
    EXPECT_EQ(fifo.size(), 0);
}

TEST_F(ExpiringFifoTest, TestPushBack) {
    fifo.push_back(1);
    EXPECT_FALSE(fifo.empty());
    EXPECT_EQ(fifo.size(), 1);
    EXPECT_EQ(fifo.front(), 1);
}

TEST_F(ExpiringFifoTest, TestExpiration) {
    auto timeout = FifoType::clock::now() + std::chrono::milliseconds(100); // Use the correct clock type
    fifo.push_back(1, timeout);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait for the item to expire
    EXPECT_TRUE(fifo.empty());
}

TEST_F(ExpiringFifoTest, TestMultipleItems) {
    auto timeout1 = FifoType::clock::now() + std::chrono::milliseconds(100);
    fifo.push_back(1, timeout1);
    
    auto timeout2 = FifoType::clock::now() + std::chrono::milliseconds(200);
    fifo.push_back(2, timeout2);
    
    auto timeout3 = FifoType::clock::now() + std::chrono::milliseconds(300);
    fifo.push_back(3, timeout3);

    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Wait for the first item to expire
    EXPECT_EQ(fifo.size(), 2);
    EXPECT_EQ(fifo.front(), 2); // The front should be 2

    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait for the second item to expire
    EXPECT_EQ(fifo.size(), 1);
    EXPECT_EQ(fifo.front(), 3); // The front should be 3

    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait for the last item to expire
    EXPECT_TRUE(fifo.empty());
}

TEST_F(ExpiringFifoTest, TestReserve) {
    fifo.reserve(10);
    for (int i = 0; i < 10; ++i) {
        fifo.push_back(i);
    }
    EXPECT_EQ(fifo.size(), 10);
}