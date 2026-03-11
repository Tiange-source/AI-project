#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <iomanip>
#include <unordered_map>

namespace test {

// 测试异常类
class TestException : public std::exception {
public:
    TestException(const std::string& message, const std::string& file, int line)
        : message_(message), file_(file), line_(line) {
    }
    
    virtual const char* what() const noexcept override {
        return message_.c_str();
    }
    
    std::string getMessage() const { return message_; }
    std::string getFile() const { return file_; }
    int getLine() const { return line_; }

private:
    std::string message_;
    std::string file_;
    int line_;
};

} // namespace test

// 测试断言宏 (定义在namespace外部，需要使用完整路径)
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            throw test::TestException("Assertion failed: " #condition, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        auto exp = (expected); \
        auto act = (actual); \
        if (exp != act) { \
            throw test::TestException("Assertion failed: " #expected " == " #actual, \
                               __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_NE(expected, actual) \
    do { \
        auto exp = (expected); \
        auto act = (actual); \
        if (exp == act) { \
            throw test::TestException("Assertion failed: " #expected " != " #actual, \
                               __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))
#define TEST_ASSERT_NULL(ptr) TEST_ASSERT_EQ(nullptr, ptr)
#define TEST_ASSERT_NOT_NULL(ptr) TEST_ASSERT_NE(nullptr, ptr)

namespace test {

// 测试用例类
class TestCase {
public:
    TestCase(const std::string& name) : name_(name) {}
    virtual ~TestCase() {}
    
    virtual void setUp() {}
    virtual void tearDown() {}
    virtual void run() = 0;
    
    const std::string& getName() const { return name_; }

protected:
    std::string name_;
};

// 测试套件类
class TestSuite {
public:
    TestSuite(const std::string& name) : name_(name) {}
    
    void addTest(TestCase* test) {
        tests_.push_back(test);
    }
    
    void runAll() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Test Suite: " << name_ << std::endl;
        std::cout << "========================================" << std::endl;
        
        int passed = 0;
        int failed = 0;
        
        for (auto* test : tests_) {
            try {
                std::cout << "\n[TEST] " << test->getName() << "... ";
                
                auto start = std::chrono::high_resolution_clock::now();
                
                test->setUp();
                test->run();
                test->tearDown();
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                
                std::cout << "PASSED (" << duration.count() << "ms)" << std::endl;
                passed++;
                
            } catch (const TestException& e) {
                std::cout << "FAILED" << std::endl;
                std::cout << "  Error: " << e.getMessage() << std::endl;
                std::cout << "  File: " << e.getFile() << ":" << e.getLine() << std::endl;
                failed++;
            } catch (const std::exception& e) {
                std::cout << "FAILED" << std::endl;
                std::cout << "  Exception: " << e.what() << std::endl;
                failed++;
            } catch (...) {
                std::cout << "FAILED" << std::endl;
                std::cout << "  Unknown exception" << std::endl;
                failed++;
            }
        }
        
        std::cout << "\n----------------------------------------" << std::endl;
        std::cout << "  Results: " << passed << " passed, " << failed << " failed" << std::endl;
        std::cout << "========================================" << std::endl;
        
        failedTests_ = failed;
    }
    
    int getFailedTests() const { return failedTests_; }

private:
    std::string name_;
    std::vector<TestCase*> tests_;
    int failedTests_ = 0;
};

// 辅助函数
inline void printBanner() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   Gomoku Server Unit Test Framework   ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
}

} // namespace test

// 测试用例宏
#define TEST_TESTCASE(suite_name, test_name) \
    class test_name##Test : public test::TestCase { \
    public: \
        test_name##Test() : test::TestCase(#test_name) {} \
        void run() override; \
    }; \
    \
    static void register_##test_name##Test() { \
        static test::TestSuite* suite = test::TestSuiteRegistry::getInstance()->getSuite(#suite_name); \
        suite->addTest(new test_name##Test()); \
    } \
    \
    static bool registered_##test_name = (register_##test_name##Test(), true); \
    \
    void test_name##Test::run()

// 测试套件注册器
namespace test {
class TestSuiteRegistry {
public:
    static TestSuiteRegistry* getInstance() {
        static TestSuiteRegistry instance;
        return &instance;
    }
    
    TestSuite* getSuite(const std::string& name) {
        if (suites_.find(name) == suites_.end()) {
            suites_[name] = new TestSuite(name);
        }
        return suites_[name];
    }
    
    void runAllSuites() {
        int totalFailed = 0;
        for (auto& pair : suites_) {
            pair.second->runAll();
            totalFailed += pair.second->getFailedTests();
        }
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Overall Result" << std::endl;
        std::cout << "========================================" << std::endl;
        
        if (totalFailed == 0) {
            std::cout << "  ✓ All tests passed!" << std::endl;
        } else {
            std::cout << "  ✗ " << totalFailed << " test(s) failed" << std::endl;
        }
        
        std::cout << "========================================\n" << std::endl;
    }
    
    int getFailedTests() {
        int totalFailed = 0;
        for (auto& pair : suites_) {
            totalFailed += pair.second->getFailedTests();
        }
        return totalFailed;
    }

private:
    TestSuiteRegistry() {}
    ~TestSuiteRegistry() {
        for (auto& pair : suites_) {
            delete pair.second;
        }
    }
    
    std::unordered_map<std::string, TestSuite*> suites_;
};
} // namespace test

#endif // TEST_FRAMEWORK_H