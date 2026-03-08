#include <cassert>
import std;
import Color;
import auxiliary_functions;

using namespace test;

int main() {

    Color A{0.1f, 1.f, 10.f};
    Color B{0.2f, 2.f, 20.f};
    float scalar = 0.13f;

    Color C=A;
    C += B;
    assert(are_close(C.r, A.r + B.r) && are_close(C.g, A.g + B.g) && are_close(C.b, A.b + B.b) && "operator+= fault");
    
    C = A + B;
    assert(are_close(C.r, A.r + B.r) && are_close(C.g, A.g + B.g) && are_close(C.b, A.b + B.b) && "operator+ fault");
    
    C = A;
    C -= B;
    assert(are_close(C.r, A.r - B.r) && are_close(C.g, A.g - B.g) && are_close(C.b, A.b - B.b) && "operator-= fault");
    
    C = A - B;
    assert(are_close(C.r, A.r - B.r) && are_close(C.g, A.g - B.g) && are_close(C.b, A.b - B.b) && "operator- fault");

    C = A;
    C *= B;
    assert(are_close(C.r, A.r * B.r) && are_close(C.g, A.g * B.g) && are_close(C.b, A.b * B.b) && "Color Color operator*= fault");

    C = A * B;
    assert(are_close(C.r, A.r * B.r) && are_close(C.g, A.g * B.g) && are_close(C.b, A.b * B.b) && "Color Color operator* fault");

    C = A;
    C *= scalar;
    assert(are_close(C.r, A.r * scalar) && are_close(C.g, A.g * scalar) && are_close(C.b, A.b * scalar) && "Color float operator*= fault");

    C = A * scalar;
    assert(are_close(C.r, A.r * scalar) && are_close(C.g, A.g * scalar) && are_close(C.b, A.b * scalar) && "Color float operator* fault");

    C = scalar * A;
    assert(are_close(C.r, A.r * scalar) && are_close(C.g, A.g * scalar) && are_close(C.b, A.b * scalar) && "float Color operator*= fault");

    C = A;
    C /= B;
    assert(are_close(C.r, A.r / B.r) && are_close(C.g, A.g / B.g) && are_close(C.b, A.b / B.b) && "Color Color operator/= fault");

    C = A / B;
    assert(are_close(C.r, A.r / B.r) && are_close(C.g, A.g / B.g) && are_close(C.b, A.b / B.b) && "Color Color operator/ fault");

    C = A;
    C /= scalar;
    assert(are_close(C.r, A.r / scalar) && are_close(C.g, A.g / scalar) && are_close(C.b, A.b / scalar) && "Color float operator/= fault");

    C = A / scalar;
    assert(are_close(C.r, A.r / scalar) && are_close(C.g, A.g / scalar) && are_close(C.b, A.b / scalar) && "Color float operator/ fault");
    

    return 0; // xmake test expects a return 0 as success hint
}