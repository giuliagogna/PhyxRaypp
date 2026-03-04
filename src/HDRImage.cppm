export module HDRImage;

import std;
import Color;

export struct HDRImage {
    int width;
    int height;
    Color* pixels;

    // Constructor
    HDRImage(int w, int h) : width(w), height(h), pixels(new Color[w * h]) {
        for (int i = 0; i < w * h; i++) {
            pixels[i]=Color(0.0, 0.0, 0.0);
        }
    }

    // Validity of pixel coordinates


    // Get pixel: gets one pixel



    // Destructor
    ~HDRImage() {
        delete[] pixels;

    }
};