import std;
import Color;
import auxiliary_functions;
import TestUtils; // Assicurati che il modulo si chiami così ora

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

    Color A{0.1f, 1.f, 10.f};
    Color B{0.2f, 2.f, 20.f};
    float scalar = 0.13f;
    Color C;

    // Sum
    C = A; C += B;
    check("operator+=", validate(C, {A.r + B.r, A.g + B.g, A.b + B.b}));
    
    check("operator+", validate(A + B, {A.r + B.r, A.g + B.g, A.b + B.b}));

    // Subtraction
    C = A; C -= B;
    check("operator-=", validate(C, {A.r - B.r, A.g - B.g, A.b - B.b}));
    
    check("operator-", validate(A - B, {A.r - B.r, A.g - B.g, A.b - B.b}));

    // Product
    C = A; C *= B;
    check("Color*Color operator*=", validate(C, {A.r * B.r, A.g * B.g, A.b * B.b}));

    check("Color*Color operator*", validate(A * B, {A.r * B.r, A.g * B.g, A.b * B.b}));

    C = A; C *= scalar;
    check("Color*float operator*=", validate(C, {A.r * scalar, A.g * scalar, A.b * scalar}));

    check("Color*float operator*", validate(A * scalar, {A.r * scalar, A.g * scalar, A.b * scalar}));

    check("float*Color operator*", validate(scalar * A, {A.r * scalar, A.g * scalar, A.b * scalar}));

    // Division
    C = A; C /= B;
    check("Color/Color operator/=", validate(C, {A.r / B.r, A.g / B.g, A.b / B.b}));

    check("Color/Color operator/", validate(A / B, {A.r / B.r, A.g / B.g, A.b / B.b}));

    C = A; C /= scalar;
    check("Color/float operator/=", validate(C, {A.r / scalar, A.g / scalar, A.b / scalar}));

    check("Color/float operator/", validate(A / scalar, {A.r / scalar, A.g / scalar, A.b / scalar}));

    std::println("\nColor algebra - - - Total tests passed: {}, Total tests failed: {}", passed_tests, failed_tests);

    return !!failed_tests; // to signal overall test success or failure
}