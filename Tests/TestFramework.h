#pragma once

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <cmath>
#include <cstdlib>

namespace Kojeom
{
struct TestResult
{
    std::string testName;
    bool passed = true;
    std::string errorMessage;
};

class TestRunner
{
public:
    using TestFn = std::function<TestResult()>;

    static TestRunner& Get()
    {
        static TestRunner instance;
        return instance;
    }

    void Register(const std::string& name, TestFn fn)
    {
        m_tests.push_back({name, fn});
    }

    int RunAll()
    {
        int passed = 0;
        int failed = 0;

        std::cout << "=== Running " << m_tests.size() << " tests ===" << std::endl;

        for (const auto& [name, fn] : m_tests)
        {
            auto result = fn();
            if (result.passed)
            {
                ++passed;
                std::cout << "  [PASS] " << name << std::endl;
            }
            else
            {
                ++failed;
                std::cout << "  [FAIL] " << name << ": " << result.errorMessage << std::endl;
            }
        }

        std::cout << "=== Results: " << passed << " passed, " << failed << " failed ===" << std::endl;
        return failed;
    }

private:
    TestRunner() = default;
    std::vector<std::pair<std::string, TestFn>> m_tests;
};

struct TestRegistrar
{
    TestRegistrar(const std::string& name, TestRunner::TestFn fn)
    {
        TestRunner::Get().Register(name, fn);
    }
};

#define KE_TEST(name) \
    static Kojeom::TestResult ke_test_fn_##name(); \
    static Kojeom::TestRegistrar ke_test_reg_##name(#name, ke_test_fn_##name); \
    static Kojeom::TestResult ke_test_fn_##name()

#define KE_EXPECT(expr) \
    do { if(!(expr)) { Kojeom::TestResult r; r.testName = __FUNCTION__; r.passed = false; \
         r.errorMessage = "Expected " #expr " at " __FILE__ ":" + std::to_string(__LINE__); return r; } } while(0)

#define KE_EXPECT_FLOAT_EQ(a, b, eps) \
    do { if(std::fabs((a) - (b)) > (eps)) { Kojeom::TestResult r; r.testName = __FUNCTION__; r.passed = false; \
         r.errorMessage = "Expected " #a " == " #b " (diff=" + std::to_string(std::fabs((a)-(b))) + ") at " __FILE__ ":" + std::to_string(__LINE__); return r; } } while(0)

#define KE_EXPECT_TRUE(expr) KE_EXPECT(expr)
#define KE_EXPECT_FALSE(expr) KE_EXPECT(!(expr))
}
