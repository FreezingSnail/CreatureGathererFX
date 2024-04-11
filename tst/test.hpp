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
    Test() : passCount(0), failCount(0) {
    }

    template <typename T> void assert(T a, T b, std::string message) {
        if (a != b) {
            ++failCount;
            failedComparisons.push_back(message + " Assertion failed: " + std::to_string(a) + " != " + std::to_string(b));
        } else {
            ++passCount;
        }
    }

    void printSummary(std::string message) {
        std::string color = (failCount > 0) ? RED : GREEN;
        std::cout << color << "---------- " << message << " ----------" << RESET << std::endl;
        std::cout << "Test: " << message << std::endl;
        std::cout << "Passed: " << passCount << "\nFailed: " << failCount << std::endl;
        if (failCount > 0) {
            std::cout << "Failed comparisons: " << std::endl;
            for (const auto &comparison : failedComparisons) {
                std::cout << RED << comparison << RESET << std::endl;
            }
        }
    }

  private:
    int passCount;
    int failCount;
    std::vector<std::string> failedComparisons;
};

void printHeader(std::string message) {
    std::cout << YELLOW << "++++++++++ " << message << " ++++++++++" << RESET << std::endl;
}