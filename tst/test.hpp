#pragma once
#include <iostream>
#include <string>
#include <vector>

// ANSI escape codes for colors
#define RED "\033[31m"
#define RESET "\033[0m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"
#define GREEN "\033[32m"

class Test {
  public:
    int failCount;
    int passCount;
    std::vector<std::string> log;

    Test() : passCount(0), failCount(0) {
    }

    Test(std::string description) : passCount(0), failCount(0), description(description) {
    }

    void addToLog(std::string message) {
        log.push_back(message);
    }

    template <typename T> void assertNotNull(T *ptr, std::string message) {
        if (ptr == nullptr) {
            ++failCount;
            failedComparisons.push_back(message + " Assertion failed: pointer is nullptr");
        } else {
            ++passCount;
        }
    }

    template <typename T> void assert(T a, T b, std::string message) {
        if (a != b) {
            ++failCount;
            failedComparisons.push_back(message + " Assertion failed: " + std::to_string(static_cast<int>(a)) +
                                        " != " + std::to_string(static_cast<int>(b)));
        } else {
            ++passCount;
        }
    }

    template <typename T1, typename T2> void assert(T1 a, T2 b, std::string message) {
        int tempA = static_cast<int>(a);
        int tempB = static_cast<int>(b);
        if (tempA != tempB) {
            ++failCount;
            failedComparisons.push_back(message + " Assertion failed: " + std::to_string(tempA) + " != " + std::to_string(tempB));
        } else {
            ++passCount;
        }
    }

    template <typename T> void assertLessThan(T a, T b, std::string message) {
        if (a >= b) {
            ++failCount;
            failedComparisons.push_back(message + " Assertion failed: " + std::to_string(a) + " >= " + std::to_string(b));
        } else {
            ++passCount;
        }
    }

    template <typename T> void assertGreaterThan(T a, T b, std::string message) {
        if (a <= b) {
            ++failCount;
            failedComparisons.push_back(message + " Assertion failed: " + std::to_string(a) + " <= " + std::to_string(b));
        } else {
            ++passCount;
        }
    }

    template <typename T1, typename T2> void assertLessThan(T1 a, T2 b, std::string message) {
        int tempA = static_cast<int>(a);
        int tempB = static_cast<int>(b);
        if (tempA >= tempB) {
            ++failCount;
            failedComparisons.push_back(message + " Assertion failed: " + std::to_string(tempA) + " >= " + std::to_string(tempB));
        } else {
            ++passCount;
        }
    }

    template <typename T1, typename T2> void assertGreaterThan(T1 a, T2 b, std::string message) {
        int tempA = static_cast<int>(a);
        int tempB = static_cast<int>(b);
        if (tempA <= tempB) {
            ++failCount;
            failedComparisons.push_back(message + " Assertion failed: " + std::to_string(tempA) + " <= " + std::to_string(tempB));
        } else {
            ++passCount;
        }
    }

    void printSummary() {
        std::string color = (failCount > 0) ? RED : GREEN;
        std::cout << color << "---------- " << description << " ----------" << RESET << std::endl;
        std::cout << "Passed: " << passCount << "\nFailed: " << failCount << std::endl;
        if (failCount > 0) {
            std::cout << "Failed comparisons: " << std::endl;
            for (const auto &comparison : failedComparisons) {
                std::cout << RED << comparison << RESET << std::endl;
            }
            if (log.size() > 0) {
                std::cout << YELLOW << "---------- " << "LOGS" << " ----------" << RESET << std::endl;
                for (int i = 0; i < log.size(); i++) {
                    std::cout << log[i] << std::endl;
                }
            }
        }
    }

  private:
    std::string description;
    std::vector<std::string> failedComparisons;
};
;

void printHeader(std::string message) {
    std::cout << YELLOW << "++++++++++ " << message << " ++++++++++" << RESET << std::endl;
}

class TestSuite {
  public:
    TestSuite(std::string description) : description(description) {
    }
    void addTest(const Test &test) {
        tests.push_back(test);
    }

    void printSummary() {
        std::cout << YELLOW << "++++++++++ " << description << " ++++++++++" << RESET << std::endl;
        for (Test &test : tests) {
            test.printSummary();
        }
    }

    bool fail() {
        for (Test &test : tests) {
            if (test.failCount > 0) {
                return true;
            }
        }
        return false;
    }

    int failCount() {
        int totalFailCount = 0;
        for (Test &test : tests) {
            totalFailCount += test.failCount;
        }
        return totalFailCount;
    }

    int passCount() {
        int totalPassCount = 0;
        for (Test &test : tests) {
            totalPassCount += test.passCount;
        }
        return totalPassCount;
    }

  private:
    std::string description;
    std::vector<Test> tests;
};

class TestRunner {
  public:
    void addTestSuite(const TestSuite &testSuite) {
        testSuites.push_back(testSuite);
    }

    void printSummary() {
        for (auto &testSuite : testSuites) {
            // Print the summary of the tests in the test suite
            testSuite.printSummary();
        }

        int totalPassCount = 0;
        int totalFailCount = 0;

        for (auto &testSuite : testSuites) {

            totalFailCount += testSuite.failCount();
            totalPassCount += testSuite.passCount();
        }
        std::cout << YELLOW << "========== " << "Total Counts" << " ==========" << RESET << std::endl;

        std::cout << "Total Passed: " << totalPassCount << "\nTotal Failed: " << totalFailCount << std::endl;
    }

    bool fail() {
        for (auto &testSuite : testSuites) {
            // If any test suite fails, return true
            if (testSuite.fail()) {
                return true;
            }
        }
        return false;
    }

  private:
    std::vector<TestSuite> testSuites;
};