#include "test_framework.h"
#include <iostream>

int main() {
    // 打印横幅
    test::printBanner();
    
    // 运行所有测试套件
    test::TestSuiteRegistry::getInstance()->runAllSuites();
    
    return test::TestSuiteRegistry::getInstance()->getFailedTests();
}