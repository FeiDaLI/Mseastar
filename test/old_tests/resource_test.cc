#include <gtest/gtest.h>
#include "../include/resource/resource.hh"
#include <memory>


class ResourceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup code
    }
    void TearDown() override {
        // Common cleanup code
    }
};

// Test memory calculation
TEST_F(ResourceTest, CalculateMemoryTest) {
    resource::configuration config;
    
    // Test with default reserve memory
    size_t available_memory = 16UL * 1024 * 1024 * 1024; // 16GB
    size_t calculated_mem = resource::calculate_memory(config, available_memory);
    EXPECT_GT(calculated_mem, 0UL);
    EXPECT_LT(calculated_mem, available_memory);

    // Test with specified total memory
    config.total_memory = 8UL * 1024 * 1024 * 1024; // 8GB
    calculated_mem = resource::calculate_memory(config, available_memory);
    EXPECT_EQ(calculated_mem, 8UL * 1024 * 1024 * 1024);

    // Test with specified reserve memory
    config = resource::configuration();
    config.reserve_memory = 2UL * 1024 * 1024 * 1024; // 2GB
    calculated_mem = resource::calculate_memory(config, available_memory);
    EXPECT_EQ(calculated_mem, available_memory - 2UL * 1024 * 1024 * 1024);

    // Test insufficient memory case
    config = resource::configuration();
    config.total_memory = 32UL * 1024 * 1024 * 1024; // 32GB
    EXPECT_THROW(resource::calculate_memory(config, available_memory), std::runtime_error);
}

// Test CPU set configuration
TEST_F(ResourceTest, CpuSetTest) {
    resource::configuration config;
    resource::cpuset cpu_set;
    
    // Test single CPU
    cpu_set.insert(0);
    config.cpu_set = cpu_set;
    auto resources = resource::allocate(config);
    EXPECT_EQ(resources.cpus.size(), 1U);
    EXPECT_EQ(resources.cpus[0].cpu_id, 0U);

    // Test multiple CPUs
    cpu_set.insert(1);
    config.cpu_set = cpu_set;
    resources = resource::allocate(config);
    EXPECT_EQ(resources.cpus.size(), 2U);
    std::set<unsigned> expected_cpus = {0, 1};
    std::set<unsigned> actual_cpus;
    for (const auto& cpu : resources.cpus) {
        actual_cpus.insert(cpu.cpu_id);
    }
    EXPECT_EQ(actual_cpus, expected_cpus);
}

// Test IO queue allocation
TEST_F(ResourceTest, IoQueueTest) {
    resource::configuration config;
    
    // Test default IO queue configuration
    auto resources = resource::allocate(config);
    EXPECT_GT(resources.io_queues.coordinators.size(), 0U);
    EXPECT_EQ(resources.io_queues.shard_to_coordinator.size(), resources.cpus.size());

    // Test specified number of IO queues
    config.io_queues = 2;
    resources = resource::allocate(config);
    EXPECT_EQ(resources.io_queues.coordinators.size(), 2U);

    // Test max IO requests
    config.max_io_requests = 256;
    resources = resource::allocate(config);
    for (const auto& coordinator : resources.io_queues.coordinators) {
        EXPECT_EQ(coordinator.capacity, 128U);  // Should be max_io_requests / num_queues
    }
}

// Test number of processing units
TEST_F(ResourceTest, ProcessingUnitsTest) {
    unsigned num_units = resource::nr_processing_units();
    EXPECT_GT(num_units, 0U);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
