#include <cassert>
#include <ostream>
import std;
import HDRImage;

int main() {
    // Creates a test image 10x10
    HDRImage img(5, 10);

    // Cases we expect to be valid
    assert(img._valid_coordinates(0, 0) == true);       // Upper left
    assert(img._valid_coordinates(4, 9) == true);       // Lower right
    assert(img._valid_coordinates(2, 7) == true);      // Casual point

    // Cases we expect not to be balid
    assert(img._valid_coordinates(-1, 5) == false);     // negative x
    assert(img._valid_coordinates(5, -1) == false);     // negative y
    assert(img._valid_coordinates(5, 5) == false);     // x over the right border
    assert(img._valid_coordinates(5, 10) == false);     // y under the lower border
    assert(img._valid_coordinates(5, 10) == false);    // x and y over the limits
    assert(img._valid_coordinates(15, 19) == false);    // casual point out the borders

    std::println("Valid coordinates are ok");

    return 0; // xmake test expects a return 0 as success hint
}