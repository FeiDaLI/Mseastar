#include <gtest/gtest.h>
#include "../include/timer/timer.hh"
#include <chrono>
#include <atomic>
#include <thread>
using namespace std::chrono_literals;

// Helper function to run engine for a short duration
void run_for(std::chrono::milliseconds duration) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < duration) {
        // Process pending events
        // In a real application, this would call engine loop
        std::this_thread::yield();
    }
}

// Test basic timer creation and destruction
TEST(TimerTest, BasicTimerOperations) {
    timer<steady_clock_type> t;
    EXPECT_FALSE(t.armed());
}

// Test setting callback and arming/canceling operations
TEST(TimerTest, TimerArming) {
    bool called = false;
    timer<steady_clock_type> t;
    
    // Test setting callback
    t.set_callback([&called] { called = true; });
    
    // // Test arming
    t.arm(1h); 
    EXPECT_TRUE(t.armed());
    
    // Test cancellation
    t.cancel();
    EXPECT_FALSE(t.armed());
}

// Test rearm operation
TEST(TimerTest, TimerRearm) {
    timer<steady_clock_type> t;
    
    // Test initial arming
    t.arm(1h);
    EXPECT_TRUE(t.armed());
    
    // Test rearming
    t.rearm(2h);
    EXPECT_TRUE(t.armed());
    
    auto timeout = t.get_timeout();
    // The timeout should be roughly 2 hours from now
    auto now = steady_clock_type::now();
    auto diff = std::chrono::duration_cast<std::chrono::minutes>(timeout - now).count();
    
    // Allow some small deviation
    EXPECT_GE(diff, 119);
    EXPECT_LE(diff, 121);
    
    t.cancel();
}

// Test arming with different duration methods
TEST(TimerTest, TimerArmMethods) {
    timer<steady_clock_type> t;
    
    // Arm with duration
    t.arm(1h);
    EXPECT_TRUE(t.armed());
    t.cancel();
    
    // Arm with time point
    auto time_point = steady_clock_type::now() + 1h;
    t.arm(time_point);
    EXPECT_TRUE(t.armed());
    t.cancel();
    
    // Arm periodic
    t.arm_periodic(1h);
    EXPECT_TRUE(t.armed());
    
    // Check that period is set (using operator bool instead of has_value())
    EXPECT_TRUE(bool(t._period));
    t.cancel();
}

// Test manual clock timer
TEST(TimerTest, ManualClockTimer) {
    bool called = false;
    timer<manual_clock> t([&called] { called = true; });
    
    // Reset manual clock counter
    manual_clock::advance(manual_clock::duration(-manual_clock::now().time_since_epoch().count()));
    
    // Arm with 100ns delay
    auto delay = manual_clock::duration(100);
    t.arm(delay);
    
    // Advance clock by 50ns - should not trigger
    manual_clock::advance(manual_clock::duration(50));
    EXPECT_FALSE(called);
    
    // Advance clock by another 51ns - should trigger
    manual_clock::advance(manual_clock::duration(51));
    // Manually expire timers since we don't have the actual engine running
    manual_clock::expire_timers();
    EXPECT_TRUE(called);
}

// Test constructor variants
TEST(TimerTest, TimerConstruction) {
    // Test default constructor
    timer<steady_clock_type> t1;
    EXPECT_FALSE(t1.armed());
    
    // Test callback constructor
    bool called = false;
    timer<steady_clock_type> t2([&called] { called = true; });
    EXPECT_FALSE(t2.armed());
    
    // Test move construction
    t2.arm(1h);
    EXPECT_TRUE(t2.armed());
    
    timer<steady_clock_type> t3(std::move(t2));
    // Original timer should be disarmed after move
    EXPECT_FALSE(t2.armed());
    
    // New timer should still be armed
    EXPECT_TRUE(t3.armed());
    t3.cancel();
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}