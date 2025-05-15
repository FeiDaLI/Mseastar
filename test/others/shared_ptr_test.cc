#include <gtest/gtest.h>
#include "../include/util/shared_ptr.hh"
#include <string>
#include <unordered_map>
#include <set>
#include <vector>

TEST(LwSharedPtrTest, BasicOperations) {
    // Test creation and basic operations.
    auto ptr = make_lw_shared<int>(42);
    EXPECT_EQ(*ptr, 42);
    EXPECT_EQ(ptr.use_count(), 1);
    // Test copy constructor.
    auto ptr2 = ptr;
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);
    
    // Test assignment.
    auto ptr3 = make_lw_shared<int>(99);
    EXPECT_EQ(*ptr3, 99);
    ptr3 = ptr;
    EXPECT_EQ(*ptr3, 42);
    EXPECT_EQ(ptr.use_count(), 3);
    
    // Test move constructor
    auto ptr4 = std::move(ptr3);
    EXPECT_EQ(*ptr4, 42);
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_FALSE(ptr3); // ptr3 should be null after move
    
    // Test nullptr assignment
    ptr2 = nullptr;
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_FALSE(ptr2);
    
    // Test value assignment
    ptr2 = 100;
    EXPECT_EQ(*ptr2, 100);
    EXPECT_EQ(ptr2.use_count(), 1);
}

// Test lw_shared_ptr with custom class
TEST(LwSharedPtrTest, CustomClassTest) {
    class TestClass {
    public:
        int value;
        TestClass(int v) : value(v) {}
    };
    
    auto ptr = make_lw_shared<TestClass>(123);
    EXPECT_EQ(ptr->value, 123);
    
    ptr->value = 456;
    EXPECT_EQ(ptr->value, 456);
}

// Test enable_lw_shared_from_this functionality
TEST(LwSharedPtrTest, EnableSharedFromThis) {
    class TestClass : public enable_lw_shared_from_this<TestClass> {
    public:
        int value;
        TestClass(int v) : value(v) {}
        
        lw_shared_ptr<TestClass> get_shared() {
            return shared_from_this();
        }
        
        lw_shared_ptr<const TestClass> get_const_shared() const {
            return shared_from_this();
        }
    };
    
    auto ptr = make_lw_shared<TestClass>(42);
    auto ptr2 = ptr->get_shared();
    
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);
    EXPECT_EQ(ptr2->value, 42);
    
    const auto& const_ref = *ptr;
    auto const_ptr = const_ref.get_const_shared();
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_EQ(const_ptr.use_count(), 3);
}

// Test shared_ptr basic functionality
TEST(SharedPtrTest, BasicOperations) {
    // Test creation and basic operations
    auto ptr = make_shared<int>(42);
    EXPECT_EQ(*ptr, 42);
    EXPECT_EQ(ptr.use_count(), 1);
    
    // Test copy constructor
    auto ptr2 = ptr;
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);
    
    // Test assignment
    auto ptr3 = make_shared<int>(99);
    EXPECT_EQ(*ptr3, 99);
    ptr3 = ptr;
    EXPECT_EQ(*ptr3, 42);
    EXPECT_EQ(ptr.use_count(), 3);
    
    // Test move constructor
    auto ptr4 = std::move(ptr3);
    EXPECT_EQ(*ptr4, 42);
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_FALSE(ptr3); // ptr3 should be null after move
    
    // Test nullptr assignment
    ptr2 = nullptr;
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_FALSE(ptr2);
}

// Test shared_ptr with custom class
TEST(SharedPtrTest, CustomClassTest) {
    class TestClass {
    public:
        int value;
        TestClass(int v) : value(v) {}
    };
    
    auto ptr = make_shared<TestClass>(123);
    EXPECT_EQ(ptr->value, 123);
    
    ptr->value = 456;
    EXPECT_EQ(ptr->value, 456);
}

// Test enable_shared_from_this functionality
TEST(SharedPtrTest, EnableSharedFromThis) {
    class TestClass : public enable_shared_from_this<TestClass> {
    public:
        int value;
        TestClass(int v) : value(v) {}
        
        shared_ptr<TestClass> get_shared() {
            return shared_from_this();
        }
        
        shared_ptr<const TestClass> get_const_shared() const {
            return shared_from_this();
        }
    };
    
    auto ptr = make_shared<TestClass>(42);
    auto ptr2 = ptr->get_shared();
    
    EXPECT_EQ(ptr.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);
    EXPECT_EQ(ptr2->value, 42);
    
    const auto& const_ref = *ptr;
    auto const_ptr = const_ref.get_const_shared();
    EXPECT_EQ(ptr.use_count(), 3);
    EXPECT_EQ(const_ptr.use_count(), 3);
}

// Test pointer casting with shared_ptr
TEST(SharedPtrTest, PointerCasting) {
    class Base {
    public:
        virtual ~Base() {}
        virtual int get_value() { return 0; }
    };
    
    class Derived : public Base {
    public:
        int value;
        Derived(int v) : value(v) {}
        int get_value() override { return value; }
    };
    
    // Test static_pointer_cast
    auto derived = make_shared<Derived>(123);
    auto base = static_pointer_cast<Base>(derived);
    EXPECT_EQ(base->get_value(), 123);
    EXPECT_EQ(derived.use_count(), 2);
    EXPECT_EQ(base.use_count(), 2);
    
    // Test dynamic_pointer_cast
    auto derived2 = dynamic_pointer_cast<Derived>(base);
    EXPECT_TRUE(derived2);
    EXPECT_EQ(derived2->value, 123);
    EXPECT_EQ(derived.use_count(), 3);
    
    // Test const_pointer_cast
    auto const_base = shared_ptr<const Base>(base);
    auto non_const = const_pointer_cast<Base>(const_base);
    EXPECT_TRUE(non_const);
    EXPECT_EQ(non_const->get_value(), 123);
}

// Test comparison operators for shared_ptr
TEST(SharedPtrTest, ComparisonOperators) {
    auto ptr1 = make_shared<int>(1);
    auto ptr2 = make_shared<int>(2);
    auto ptr3 = ptr1;
    
    EXPECT_TRUE(ptr1 == ptr3);
    EXPECT_FALSE(ptr1 == ptr2);
    EXPECT_TRUE(ptr1 != ptr2);
    EXPECT_FALSE(ptr1 != ptr3);
    
    EXPECT_TRUE(ptr1 < ptr2 || ptr2 < ptr1); // Pointer address comparison
    
    EXPECT_FALSE(ptr1 == nullptr);
    EXPECT_TRUE(ptr1 != nullptr);
    EXPECT_TRUE(nullptr != ptr1);
    EXPECT_FALSE(nullptr == ptr1);
}

// Test indirect comparison and hashing utilities
TEST(SharedPtrTest, IndirectUtilities) {
    // Test with a set using indirect_less
    std::set<shared_ptr<std::string>, indirect_less<shared_ptr<std::string>>> str_set;
    
    auto str1 = make_shared<std::string>("abc");
    auto str2 = make_shared<std::string>("def");
    auto str3 = make_shared<std::string>("abc"); // Same value as str1
    
    str_set.insert(str1);
    str_set.insert(str2);
    str_set.insert(str3);
    
    // Should only have 2 elements because str1 and str3 have the same value
    EXPECT_EQ(str_set.size(), 2);
    
    // Test with unordered_map using indirect_hash and indirect_equal_to
    std::unordered_map<
        shared_ptr<std::string>,
        int,
        indirect_hash<shared_ptr<std::string>>,
        indirect_equal_to<shared_ptr<std::string>>
    > str_map;
    
    str_map[str1] = 1;
    str_map[str2] = 2;
    
    // str3 should map to the same entry as str1
    EXPECT_EQ(str_map[str3], 1);
    EXPECT_EQ(str_map.size(), 2);
}

// Test owned() functionality of lw_shared_ptr
TEST(LwSharedPtrTest, OwnedTest) {
    auto ptr = make_lw_shared<int>(42);
    EXPECT_TRUE(ptr.owned());
    
    auto ptr2 = ptr;
    EXPECT_FALSE(ptr.owned());
    EXPECT_FALSE(ptr2.owned());
    
    ptr2 = nullptr;
    EXPECT_TRUE(ptr.owned());
}

// Test make_shared with different constructor arguments
TEST(SharedPtrTest, MakeShared) {
    // Make with a simple value
    auto ptr1 = make_shared<int>(42);
    EXPECT_EQ(*ptr1, 42);
    
    // With custom class to avoid ambiguous overloads
    class TestString {
    public:
        std::string value;
        TestString(const char* s) : value(s) {}
        bool operator==(const std::string& other) const { return value == other; }
    };
    
    auto ptr2 = make_shared<TestString>("hello");
    EXPECT_TRUE(ptr2->value == "hello");
    
    // Make with a vector using default constructor and push_back
    auto ptr3 = make_shared<std::vector<int>>();
    ptr3->push_back(1);
    ptr3->push_back(2);
    ptr3->push_back(3);
    EXPECT_EQ(ptr3->size(), 3);
    EXPECT_EQ((*ptr3)[0], 1);
    EXPECT_EQ((*ptr3)[1], 2);
    EXPECT_EQ((*ptr3)[2], 3);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
