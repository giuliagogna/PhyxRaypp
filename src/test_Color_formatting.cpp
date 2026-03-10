#include <cassert>
import std;
import Color;
import auxiliary_functions;
import TestUtils;

using namespace test;
using namespace aux;

int main() {

    int passed_tests{0}, failed_tests{0};
    auto check = [&](std::string_view test_name, auto validation_result) {
        if (!validation_result) {
            std::string_view error_msg = error_to_string(validation_result.error());
            std::println("Test failed: {} - {}", test_name, error_msg);
            failed_tests++;
        } else {
            std::println("Test passed: {}", test_name);
            passed_tests++;
        }
    };

    Color A{0.12345f, 1.23456f, 12.34567f};
    std::string expected(std::format("Aaa {:.3f} Aaa", A));
    std::string actual(std::format("Aaa {:.3f} {:.3f} {:.3f} Aaa", A.r, A.g, A.b));

    check("Color formatting", validate(actual, expected));

    std::println("Color formatting - - - Passed {} tests, Failed {} tests", passed_tests, failed_tests);
    
    return !!failed_tests; // xmake test expects a return 0 as success hint
}