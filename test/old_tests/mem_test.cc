#include <gtest/gtest.h>
#include "../include/mem/memory.hh"
#include "../include/resource/resource.hh"
#include <vector>
#include <thread>
#include <algorithm>
#include <iostream>
#include <random>
class MemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure memory with a single NUMA node and 64MB of memory
        std::vector<resource::memory> mem_config = {
            {.bytes = 1024 * 1024, .nodeid = 0}
        };
        memory::configure(mem_config, {});
    }
    void TearDown() override {
    }
};

// Test basic allocation and deallocation
TEST_F(MemoryTest, BasicAllocation) {
    // Test small allocation
    void* small_ptr = memory::allocate(100);
    ASSERT_NE(nullptr, small_ptr);
    memory::free(small_ptr);
    // Test large allocation
    void* large_ptr = memory::allocate(1024 * 1024);
    ASSERT_NE(nullptr, large_ptr);
    memory::free(large_ptr);
}

// Test aligned allocation
TEST_F(MemoryTest, AlignedAllocation) {
    size_t alignment = 4096; // Page size
    void* ptr = memory::allocate_aligned(alignment, 1000);
    ASSERT_NE(nullptr, ptr);
    // Check alignment
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    EXPECT_EQ(0u, addr % alignment);    
    memory::free(ptr);
}

// Test object size query
TEST_F(MemoryTest, ObjectSize) {
    // Allocate memory and check its size
    size_t requested_size = 1000;
    void* ptr = memory::allocate(requested_size);
    ASSERT_NE(nullptr, ptr);
    size_t actual_size = memory::object_size(ptr);
    EXPECT_GE(actual_size, requested_size);
    memory::free(ptr);
}

// Test memory shrinking
TEST_F(MemoryTest, Shrink) {
    // Allocate a large block and shrink it
    size_t initial_size = 1024 * 1024; // 1MB
    void* ptr = memory::allocate(initial_size);
    ASSERT_NE(nullptr, ptr);

    size_t new_size = 512 * 1024; // 512KB
    memory::shrink(ptr, new_size);
    size_t actual_size = memory::object_size(ptr);
    EXPECT_GE(actual_size, new_size);
    EXPECT_LE(actual_size, initial_size);
    memory::free(ptr);
}

// Test multiple allocations and deallocations
TEST_F(MemoryTest, MultipleAllocations) {
    std::vector<void*> ptrs;
    const int num_allocs = 100;
    
    // Perform multiple allocations
    for (int i = 0; i < num_allocs; ++i) {
        void* ptr = memory::allocate(1024); // 1KB each
        ASSERT_NE(nullptr, ptr);
        ptrs.push_back(ptr);
    }
    // Free in random order
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ptrs.begin(), ptrs.end(), g);
    for (void* ptr : ptrs) {
        memory::free(ptr);
    }
}

// Test memory statistics
TEST_F(MemoryTest, Statistics) {
    // Get initial stats
    auto initial_stats = memory::stats();
    
    // Perform some allocations
    void* ptr1 = memory::allocate(1024);
    void* ptr2 = memory::allocate(2048);
    
    // Get stats after allocations
    auto after_alloc_stats = memory::stats();
    EXPECT_GT(after_alloc_stats.mallocs(), initial_stats.mallocs());
    EXPECT_GT(after_alloc_stats.allocated_memory(), initial_stats.allocated_memory());
    
    // Free memory
    memory::free(ptr1);
    memory::free(ptr2);
    
    // Get final stats
    auto final_stats = memory::stats();
    EXPECT_GT(final_stats.frees(), initial_stats.frees());
}

// Test allocation failure handling
TEST_F(MemoryTest, AllocationFailure) {
    // Try to allocate more memory than available.
    size_t huge_size = 1ULL << 40; // 1TB
    EXPECT_THROW(memory::allocate(huge_size), std::bad_alloc);
}

// Test cross-CPU operations
TEST_F(MemoryTest, CrossCPUOperations) {
    void* ptr = memory::allocate(1024);
    ASSERT_NE(nullptr, ptr);
    
    // Drain cross-CPU freelists and check result
    bool drained = memory::drain_cross_cpu_freelist();
    
    // Free the pointer
    memory::free(ptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
