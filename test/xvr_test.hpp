#ifndef XVR_TEST_HPP
#define XVR_TEST_HPP

#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#define TEST_CASE(name) void test_##name()
#define ASSERT_TRUE(expr) assert(expr)
#define ASSERT_FALSE(expr) assert(!(expr))
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define ASSERT_NULL(ptr) assert((ptr) == nullptr)
#define ASSERT_NOT_NULL(ptr) assert((ptr) != nullptr)

namespace xvr {
namespace test {

class TestCase {
public:
    using TestFn = void(*)();
    
    TestCase(std::string_view name, TestFn fn) : name_(name), fn_(fn) {}
    
    void run() {
        std::cout << "Running: " << name_ << "... ";
        try {
            fn_();
            std::cout << "PASSED\n";
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << "\n";
            throw;
        }
    }
    
    std::string_view name() const { return name_; }
    
private:
    std::string_view name_;
    TestFn fn_;
};

class TestRegistry {
public:
    static TestRegistry& instance();
    
    void registerTest(std::string_view suite, std::string_view name, TestCase::TestFn fn);
    void runAll();
    void runSuite(std::string_view suite);
    
private:
    TestRegistry() = default;
    
    struct TestEntry {
        std::string suite;
        std::string name;
        TestCase::TestFn fn;
    };
    
    std::vector<TestEntry> tests_;
};

#define TEST_SUITE(suite_name) namespace suite_name {
#define END_SUITE }

#define REGISTER_TEST(suite, name) \
    static bool _test_##suite##_##name = ([]() { \
        xvr::test::TestRegistry::instance().registerTest(#suite, #name, test_##name); \
        return true; \
    })();

}  // namespace test
}  // namespace xvr

#endif  // XVR_TEST_HPP