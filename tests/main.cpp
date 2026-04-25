#include "runner.h"

int main() {
    std::cout << "Algo-Catalyst Test Suite\n";
    std::cout << "========================\n\n";
    TestRunner::runAll();
    return TestRunner::failures > 0 ? 1 : 0;
}
