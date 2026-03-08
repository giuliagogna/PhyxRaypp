export module auxiliary_functions;

import std;

export namespace test {

    bool are_close(float x, float y, float tolerance=10e-5f) {
        return std::abs(x - y) <= tolerance;

    }
}