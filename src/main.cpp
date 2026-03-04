import std;
import HDRImage;
import Color;

using namespace std;

int main(int argc, char **argv) {
    HDRImage img(10, 11);

    int x=9;
    int y=10;

    Color pixel_0 = img.get_pixel(x, y);
    std::println("Colore del pixel [{}{}] predefinito: {}", x, y, pixel_0.to_string());

    Color c(0.2, 0.4, 6.9);
    std::println("Colore c: {}", c.to_string());

    img.set_pixel(x, y, c);
    Color pixel_1 = img.get_pixel(x, y);
    std::println("Nuovo colore del pixel [{}{}]: {}", x, y, pixel_1.to_string());



    return 0;
}
