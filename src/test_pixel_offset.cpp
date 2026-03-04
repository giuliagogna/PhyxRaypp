#include <cassert>
import std;
import HDRImage;

int main() {

    HDRImage img(5, 7);

    // Favourable cases
    assert(img._pixel_offset(3, 2) == 2*5 + 3);
    assert(img._pixel_offset(0, 0) == 0);
    assert(img._pixel_offset(4, 6) == 34);


    return 0; // xmake test expects a return 0 as success hint
}