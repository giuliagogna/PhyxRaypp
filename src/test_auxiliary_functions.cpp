#include <cassert>
import auxiliary_functions;
import std;
import TestUtils;

using namespace test;
using namespace aux;

int main() {

    // Test are_close
    float x = 0.1;
    float sum = 0.0;

    for(int i = 0; i < 10; i++) {
        sum += x;
    }

    assert(are_close(sum, 1)==true);

    for(int i = 0; i < 20; i++) {
        sum -= x;
    }

    assert(are_close(sum, 1)==false);



    // Final print
    std::println("Auxiliary functions ok.");

    return 0;
}