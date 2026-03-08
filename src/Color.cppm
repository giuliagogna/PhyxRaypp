module;
#include <assert.h>
export module Color;
import std;


export struct Color {
    float r = 0.f, g = 0.f, b = 0.f;
    
    // If assert is not used, I'll just avoid rewriting it
//    constexpr Color(float red, float green, float blue) : r(red), g(green), b(blue) {
//        
//        // I would ask Color members to be defined positive if they represent my data set.
//        // However, I'm not sure if in intermediate calculations
//        // every Color object would behave as well.
//        // Anyway, it gives overhead so I'll probably leave this commented...
//        
//        //assert(red >= 0.f &&  green >= 0.f && blue >= 0.f && "Illegal color");
//    };

    //sum
    constexpr Color& operator+=(const Color& other) {
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }
    //sum
    friend constexpr Color operator+(Color lvalue, const Color& rvalue) {
        lvalue += rvalue;
        return lvalue;
    }


    // difference
    constexpr Color& operator-=(const Color& other) {
        r -= other.r;
        g -= other.g;
        b -= other.b;
        return *this;
    }

    // difference
    friend constexpr Color operator-(Color lvalue, const Color& rvalue) {
        lvalue -= rvalue;
        return lvalue;
    }


    // Commutative and distributive because
    // it's commutative and ditributive component-wise.

    // component-wise product
    constexpr Color& operator*=(const Color& other) {
        r *= other.r;
        g *= other.g;
        b *= other.b;
        return *this;
    }

    // component-wise product
    friend constexpr Color operator*(Color lvalue, const Color& rvalue) {
        lvalue *= rvalue;
        return lvalue;
    }


    // product with scalar (float)
    constexpr Color& operator*=(const float scalar) {
        r *= scalar;
        g *= scalar;
        b *= scalar;
        return *this;
    }

    // product with scalar (float)
    friend constexpr Color operator*(Color lvalue, const float scalar) {
        lvalue *= scalar;
        return lvalue;
    }

    // product with scalar (float)
    friend constexpr Color operator*(const float scalar, Color rvalue) {
        rvalue *= scalar;
        return rvalue;
    }


    // I'll probabliy leave the native /=0 float check to avoid overhead.
    // I'll probably never use it actually.

    // division component-wise (not really interesting)
    constexpr Color& operator/=(const Color& other) {
        //assert(other.r != 0.f && other.g != 0.f && other.b != 0.f && "Illegal component-wise division")
        r /= other.r;
        g /= other.g;
        b /= other.b;
        return *this;
    }

    // division component-wise (not really interesting)
    friend constexpr Color operator/(Color lvalue, const Color& rvalue) {
        lvalue /= rvalue;
        return lvalue;
    }


    // I'll probabliy leave the native /=0 float check to avoid overhead.

    // division by scalar (float)
    constexpr Color& operator/=(const float scalar) {
        //assert(scalar != 0.f && "Illegal scalar division")
        float inverse = 1.f / scalar;
        r *= inverse;
        g *= inverse;
        b *= inverse;
        return *this;
    }

    // division by scalar (float)
    friend constexpr Color operator/(Color lvalue, const float scalar) {
        lvalue /= scalar;
        return lvalue;
    }


    // Convert the RGB triplet in a string and displays
    std::string to_string() const {
        return std::format("{} {} {}", r, g, b);
    }

};