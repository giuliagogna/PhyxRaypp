#include <cassert>
import std;
import HDRImage;
import Color;
import auxiliary_functions;

int main() {

    HDRImage img(6, 10);

    float r=0.2;
    float g=0.8;
    float b=6.9;

    Color c(r, g, b);

    int x = 5;
    int y = 8;

    img.set_pixel(x, y, c);

    Color pixel_color = img.get_pixel(x, y);

    assert(are_close(pixel_color.r, c.r) == true);
    assert(are_close(pixel_color.g, c.g) == true);
    assert(are_close(pixel_color.b, c.b) == true);

    std::println("Pixel color values are correct");


    return 0;
}