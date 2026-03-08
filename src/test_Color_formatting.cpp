#include <cassert>
import std;
import Color;
import auxiliary_functions;

using namespace test;

int main() {

    Color A{0.12345f, 1.23456, 12.34567};
    std::string s = std::format("Aaa {:.3f} Aaa", A);
    assert (s == "Aaa 0.123 1.235 12.346 Aaa" && "format fault");
    return 0;
    
}