module;
#include <cassert>

export module HDRImage;

import std;
import Color;


export struct HDRImage {
    int width;
    int height;
    std::vector<Color> pixels;

    // Constructor
    HDRImage(int w, int h) : width(w), height(h), pixels(w * h, Color(0.0, 0.0, 0.0)) {};

    // Checks validity of pixel coordinates
    bool _valid_coordinates(int x, int y) {
        return (x >= 0 && x < width && y >= 0 && y < height);
    }

    // Calculates pixel position from 2D coordinates
    int _pixel_offset(int x, int y) {
        assert(_valid_coordinates(x, y) && "Coordinates are out of border");
        return x + (y * width);
    }

    //Gets the pixel given the coordinates
    Color get_pixel(int x, int y) {
        assert(_valid_coordinates(x, y) && "Coordinates are out of border");
        return pixels[_pixel_offset(x, y)];
    }

    void set_pixel(int x, int y, Color c) {
        assert(_valid_coordinates(x, y) && "Coordinates are out of border");
        pixels[_pixel_offset(x, y)] = c;
    }




    // Destructor
    ~HDRImage() = default;
};