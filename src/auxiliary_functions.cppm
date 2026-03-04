export module auxiliary_functions;

import std;

export bool are_close(float x, float y, float tolerance=10e-5) {
    return std::abs(x - y) <= tolerance;

}